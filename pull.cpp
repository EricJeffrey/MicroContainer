#if !defined(PULL_CPP)
#define PULL_CPP

#include "pull.h"
#include "extract.h"
#include "image_repo.h"
#include "utils.h"

#include <filesystem>
#include <fstream>

typedef std::pair<int, nlohmann::json> PairIntJson;

string getRegistryPath(RegistryEndPoint endPoint, std::vector<string> args) {
    std::stringstream ss;
    switch (endPoint) {
        case CHECK:
            ss << "/v2/";
            break;
        case IMAGE_MANIFESTS:
            ss << "/v2/library/" << args[0] << "/manifests/" << args[1];
            break;
        case IMAGE_BLOBS:
            ss << "/v2/library/" << args[0] << "/blobs/" << args[1];
            break;
        default:
            break;
    }
    return ss.str();
}

// return <manifest, status-code, error>
TupStrIntBool getManifest(httplib::Client &client, const string &imgName, const string &tag) {
    httplib::Headers headers(
        {std::make_pair("Accept", "application/vnd.docker.distribution.manifest.v2+json")});
    auto resp = client.Get(
        getRegistryPath(RegistryEndPoint::IMAGE_MANIFESTS, {imgName, tag}).c_str(), headers);
    if (resp.error() || resp->status != 200)
        return std::make_tuple("", resp->status, resp.error());
    return std::make_tuple(resp->body, 200, false);
}

// FIXME Fetch image v2 manifest failed: SignatureDoesNotMatch
// throw httplib.client error
void fetchBlobs(httplib::Client &client, const ImageData &imageData, const string &imgName) {
    int blobSetSz = imageData.layerBlobSumSet.size(), blobCntK = 1;
    for (auto &&blobSum : imageData.layerBlobSumSet) {
        if (std::filesystem::is_directory(OVERLAY_DIR_PATH + blobSum.substr(7))) {
            std::cerr << "\r" << blobCntK << "/" << blobSetSz << " " << blobSum.substr(7, 16)
                      << ": local" << std::endl;
            continue;
        }
        usleep(800000);
        const string blobAddr = getRegistryPath(RegistryEndPoint::IMAGE_BLOBS, {imgName, blobSum});
        int cntRetry = 0;
        for (; cntRetry < FetchBlobMaxRetryTimes; cntRetry++) {
            usleep(80000);
            bool ok = true;
            size_t totalSize = 0, receivedSize = 0;
            ofstream layerFStream(LAYER_FILE_DIR_PATH + blobSum.substr(7) + ".tar.gz",
                                  std::ios::out);
            httplib::Result blobResp = client.Get(
                blobAddr.c_str(), httplib::Headers(),
                [&](const httplib::Response &tmpResp) {
                    if (tmpResp.status != 200 && tmpResp.status / 100 != 3) {
                        loggerInstance()->warn("Invalid response code, retrying");
                        ok = false;
                        return false;
                    }
                    totalSize = std::stoul(tmpResp.headers.find("Content-Length")->second);
                    return true;
                },
                [&](const char *data, size_t dataLength) {
                    layerFStream.write(data, dataLength);
                    receivedSize += dataLength;
                    // todo change to logger::raw or something like that
                    std::cerr << "\r" << blobCntK << "/" << blobSetSz << " "
                              << blobSum.substr(7, 16) << ": "
                              << receivedSize * 1.0 / totalSize * 100 << "%";
                    return true;
                });
            layerFStream.flush();
            layerFStream.close();
            if (ok)
                break;
        }
        if (cntRetry >= FetchBlobMaxRetryTimes) {
            loggerInstance()->error("Fetch image layer failed: Exceed max retry times");
            throw runtime_error("exceed max retry times");
        }
        ++blobCntK;
        std::cerr << std::endl;
    }
}

// return <errCode, configJson>
PairIntJson fetchV2Config(httplib::Client &client, const ImageData &imageData,
                          const string &imgName, const string &tag) {
    using std::make_pair;
    auto errorPair = make_pair(-1, "");
    // get config directly
    auto imgConfBlobResp = client.Get(
        getRegistryPath(RegistryEndPoint::IMAGE_BLOBS, {imgName, imageData.confBlobDigest})
            .c_str());
    if (imgConfBlobResp.error()) {
        loggerInstance()->error("Get image configuration failed:", imgConfBlobResp.error());
        return errorPair;
    }
    if (imgConfBlobResp->status == 200) {
        try {
            auto resJson = nlohmann::json::parse(imgConfBlobResp->body);
            return make_pair(0, nlohmann::json::parse(imgConfBlobResp->body));
        } catch (const std::exception &e) {
            loggerInstance()->error("Parsing v2 configuration failed:", e.what());
            return errorPair;
        }
    } else if (imgConfBlobResp->status == 404) {
        // fall back to v1
        auto imgV1ManifestResp = client.Get(
            getRegistryPath(RegistryEndPoint::IMAGE_MANIFESTS, {imgName, tag}).c_str(),
            httplib::Headers(
                {make_pair("Accept", "application/vnd.docker.distribution.manifest.v1+json")}));
        if (imgV1ManifestResp.error()) {
            loggerInstance()->error("Fetch image v1 manifest failed:", imgV1ManifestResp.error());
            return errorPair;
        }
        if (imgV1ManifestResp->status == 200) {
            try {
                using nlohmann::json;
                return make_pair(
                    0, json::parse(imgV1ManifestResp->body)["history"][0]["v1Compatibility"]);
            } catch (const std::exception &e) {
                loggerInstance()->error("Parsing v1 manifest failed:", e.what());
                return errorPair;
            }
        } else {
            loggerInstance()->error("Fetch image v1 manifst failed, invalid response code:",
                                    imgV1ManifestResp->status);
            return errorPair;
        }
    } else {
        loggerInstance()->error("Fetch image v2 manifest failed, invalid response code:",
                                imgConfBlobResp->status);
        loggerInstance()->debug("Response: ", [&imgConfBlobResp]() {
            string res = "{ ";
            for (auto &&[key, value] : imgConfBlobResp->headers)
                res += "{" + key + ", " + value + "},";
            res += " }\n\t" + imgConfBlobResp->body;
            return res;
        }());
        return errorPair;
    }
}

// No throw
int pull(const string &imgName, const string &tag, const string &regAddr) noexcept {
    using std::get, std::make_shared, std::shared_ptr;

    // check local existence
    loggerInstance()->info("Checking", imgName + ":" + tag, "locally");
    try {
        ImageRepo imageRepo;
        imageRepo.open(IMAGE_REPO_DB_PATH);
        // find locally
        if (imageRepo.contains(imgName, tag)) {
            loggerInstance()->info("Find", imgName + ":" + tag, "locally, exiting");
            return 0;
        }
    } catch (const std::exception &e) {
        loggerInstance()->error("Failed to open database:", e.what());
        return -1;
    }

    // fetch from registry
    loggerInstance()->info("Fetching image", imgName + ":" + tag);
    httplib::Client client(regAddr.c_str());
    client.set_read_timeout(ReadTimeoutInSec);
    // check connection
    try {
        loggerInstance()->info("Connecting to registry:", regAddr);
        auto resp = client.Get(getRegistryPath(RegistryEndPoint::CHECK).c_str());
        if (resp.error() != httplib::Success) {
            loggerInstance()->error("Connect to registry:", regAddr, "failed");
            return -1;
        }
        if (resp->status != 200) {
            loggerInstance()->error("Cannot fetch image from registry:", regAddr,
                                    "v2 not supported");
            return -1;
        }
    } catch (const std::exception &e) {
        loggerInstance()->error("Connect to registry failed:", e.what());
        return -1;
    }

    // get manifest
    loggerInstance()->info("Fetching image manifest");
    string manifestRaw;
    {
        auto getManifestRes = getManifest(client, imgName, tag);
        if (get<2>(getManifestRes) || get<1>(getManifestRes) != 200) {
            if (get<2>(getManifestRes))
                loggerInstance()->error("Fetch image manifest from registry:", regAddr, "failed");
            else
                loggerInstance()->info("Image:", imgName + ":" + tag, "not found");
            return -1;
        }
        manifestRaw = get<0>(getManifestRes);
    }

    // parse manifest
    int manitSchemaVer = 0;
    ImageData imageData;
    try {
        manitSchemaVer = imageData.buildFromRaw(manifestRaw);
        if (manitSchemaVer != 1 && manitSchemaVer != 2) {
            loggerInstance()->error("Unknown manifest version:", manitSchemaVer);
            return -1;
        }
    } catch (const std::exception &e) {
        loggerInstance()->error("Parsing manifest failed:", e.what());
        return -1;
    }

    // fetch blobs
    loggerInstance()->info("Fetching image layers");
    try {
        client.set_follow_location(true);
        std::cerr << std::fixed << std::setprecision(2);
        fetchBlobs(client, imageData, imgName);
    } catch (const std::exception &e) {
        loggerInstance()->error("Fetch image layer failed:", e.what());
        return -1;
    }

    // extract
    loggerInstance()->info("Extracting layers");
    int blobSetSz = imageData.layerBlobSumSet.size(), blobCntK = 1;
    for (auto &&blobSum : imageData.layerBlobSumSet) {
        const string filePath = LAYER_FILE_DIR_PATH + blobSum.substr(7) + ".tar.gz";
        const string dstDirPath = OVERLAY_DIR_PATH + blobSum.substr(7) + "/";
        try {
            std::cerr << (blobCntK) << "/" << blobSetSz << " " << blobSum.substr(7, 16) << "...";
            auto extractRet = extract(filePath, dstDirPath);
            ++blobCntK;
            if (extractRet.first == -1) {
                loggerInstance()->error("call to extract failed:", extractRet.second);
                return -1;
            } else {
                std::cerr << "done\n";
                if (extractRet.first == -2)
                    loggerInstance()->warn("extract returned with warn:", extractRet.second);
            }
        } catch (const std::exception &e) {
            loggerInstance()->error("extract layer failed:", e.what());
            return -1;
        }
    }

    // fetch config, only need on schemaV2
    loggerInstance()->info("Fetching image configuration");
    if (manitSchemaVer == 2) {
        // get config directly
        client.set_follow_location(true);
        auto fetchConfigRes = fetchV2Config(client, imageData, imgName, tag);
        if (fetchConfigRes.first != 0)
            return -1;
        imageData.config = fetchConfigRes.second;
    } else if (manitSchemaVer != 1) {
        loggerInstance()->error("Unknown manifest version");
        return -1;
    }

    loggerInstance()->info("Storing image");
    // make image dir
    const string imageID = imageData.confBlobDigest.substr(7);
    int ret = mkdir((IMAGE_DIR_PATH + imageID).c_str(), S_IRWXU | S_IRWXG | S_IROTH);
    if (ret == -1) {
        loggerInstance()->sysError(errno, "Make image dir:", IMAGE_DIR_PATH + imageID, "failed");
        return -1;
    }
    // store image manifest
    try {
        const string formattedManifest = imageData.manifest.dump(4);
        ofstream manifestOFStream(IMAGE_DIR_PATH + imageID + "/manifest.json", std::ios::out);
        manifestOFStream.write(formattedManifest.c_str(), formattedManifest.size());
    } catch (const std::exception &e) {
        loggerInstance()->error("Write manifest failed:", e.what());
        return -1;
    }
    // store image config
    try {
        const string formateedConfig = imageData.config.dump(4);
        ofstream imgConfigOFStream(IMAGE_DIR_PATH + imageID + "/config.json", std::ios::out);
        imgConfigOFStream.write(formateedConfig.c_str(), formateedConfig.size());
    } catch (const std::exception &e) {
        loggerInstance()->error("Write image configuration failed:", e.what());
        return -1;
    }
    // store image info
    {
        ImageRepoItem item(DEFAULT_REG_ADDR_ABBR, imgName, tag, imageID, now(), 0);
        ImageRepo imageRepo;
        imageRepo.open(IMAGE_REPO_DB_PATH);
        imageRepo.addImg(imageID, item);
    }
    loggerInstance()->info("Image", imgName + ":" + tag, "stored at:", IMAGE_DIR_PATH + imageID);
    return 0;
}

int pull(const string &imgNameTag, const string &regAddr) noexcept {
    string imgName, tag;
    const size_t colonPos = imgNameTag.find(':');
    if (colonPos != imgNameTag.npos) {
        imgName = imgNameTag.substr(0, colonPos);
        tag = imgNameTag.substr(colonPos + 1);
    } else {
        imgName = imgNameTag, tag = "latest";
    }
    return pull(imgName, tag, regAddr);
}

#endif // PULL_CPP
