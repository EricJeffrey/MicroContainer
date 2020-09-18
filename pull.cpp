#if !defined(PULL_CPP)
#define PULL_CPP

#include "pull.h"
#include "extract.h"
#include "imagerepo.h"

#include <fstream>

string sha256_string(const char *str, size_t len) {
    char output_buffer[65];
    std::fill(output_buffer, output_buffer + 65, 0);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, len);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_buffer + (i * 2), "%02x", hash[i]);
    }
    return string(output_buffer);
}

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

// check connection, return <http-status-code, client.error()>
PairIntBool checkV2Conn(httplib::Client &client) {
    auto resp = client.Get(getRegistryPath(RegistryEndPoint::CHECK).c_str());
    return std::make_pair(resp->status, resp.error());
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

// throw httplib.client error
void fetchBlobs(httplib::Client &client, const ImageData &imageData, const string &imgName) {
    int blobSetSz = imageData.layerBlobSumSet.size(), blobCntK = 1;
    for (auto &&blobSum : imageData.layerBlobSumSet) {
        size_t totalSize = 0, receivedSize = 0;
        const string blobAddr = getRegistryPath(RegistryEndPoint::IMAGE_BLOBS, {imgName, blobSum});
        ofstream layerFStream(layerDirPath + blobSum.substr(7) + ".tar.gz", std::ios::out);
        httplib::Result blobResp = client.Get(
            blobAddr.c_str(), httplib::Headers(),
            [&](const httplib::Response &tmpResp) {
                if (tmpResp.status != 200 && tmpResp.status / 100 != 3) {
                    loggerInstance()->error("Fetch image layer failed, invalid response code");
                    return false;
                }
                totalSize = std::stoul(tmpResp.headers.find("Content-Length")->second);
                return true;
            },
            [&](const char *data, size_t dataLength) {
                layerFStream.write(data, dataLength);
                receivedSize += dataLength;
                // todo change to logger::raw, use function template
                std::cerr << "\r" << blobCntK << "/" << blobSetSz << " " << blobSum.substr(7, 16)
                          << ": " << receivedSize * 1.0 / totalSize * 100 << "%";
                return true;
            });
        layerFStream.flush();
        layerFStream.close();
        ++blobCntK;
        std::cerr << std::endl;
    }
}

// return <errCode, configJson>, log on process
PairIntJson fetchV2Config(httplib::Client &client, const ImageData &imageData,
                          const string &imgName, const string &tag) {
    using std::make_pair;
    auto errorPair = make_pair(-1, "");
    // get config directly
    auto imgConfigBlobResp = client.Get(
        getRegistryPath(RegistryEndPoint::IMAGE_BLOBS, {imgName, imageData.configBlobDigest})
            .c_str());
    if (imgConfigBlobResp.error()) {
        loggerInstance()->error("Get image configuration failed:", imgConfigBlobResp.error());
        return errorPair;
    }
    if (imgConfigBlobResp->status == 200) {
        try {
            auto resJson = nlohmann::json::parse(imgConfigBlobResp->body);
            return make_pair(0, nlohmann::json::parse(imgConfigBlobResp->body));
        } catch (const std::exception &e) {
            loggerInstance()->error("Parsing v2 configuration failed:", e.what());
            return errorPair;
        }
    } else if (imgConfigBlobResp->status == 404) {
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
                                imgConfigBlobResp->status);
        return errorPair;
    }
}

int pull(const string &imgNameTag, const string &regAddr) {
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

// No throw
int pull(const string &imgName, const string &tag, const string &regAddr) {
    using std::get;

    // check local existence
    loggerInstance()->info("Checking", imgName + ":" + tag, "locally");
    ImageRepoInfo repoInfo;
    try {
        // may need lock file first
        std::ifstream repoFStream(imageDirPath + imageRepoFileName);
        string repoJsonStr((std::istreambuf_iterator<char>(repoFStream)),
                           std::istreambuf_iterator<char>());
        repoFStream.close();
        if (repoJsonStr.empty())
            repoJsonStr = "{}";
        repoInfo = ImageRepoInfo::buildFromJson(nlohmann::json::parse(repoJsonStr));
    } catch (const std::exception &e) {
        loggerInstance()->error("Cannot read local repo info:", e.what());
        return -1;
    }
    if (repoInfo.contains(imgName, tag)) {
        // find locally
        loggerInstance()->info("Find", imgName + ":" + tag, "locally, exiting");
        return 0;
    }

    // fetch from registry
    loggerInstance()->info("Fetching image", imgName + ":" + tag);
    httplib::Client client(regAddr.c_str());
    client.set_read_timeout(readTimeoutInSec);

    // check connection
    loggerInstance()->info("Connecting to registry:", regAddr);
    auto checkConnRes = checkV2Conn(client);
    if (checkConnRes.first != 200 || checkConnRes.second) {
        if (checkConnRes.second)
            loggerInstance()->error("Connect to registry:", regAddr, "failed");
        else
            loggerInstance()->error("Cannot fetch image from registry:", regAddr,
                                    "v2 not supported");
        return -1;
    }

    // get manifest
    loggerInstance()->info("Fetching image manifest");
    auto getManifestRes = getManifest(client, imgName, tag);
    if (get<2>(getManifestRes) || get<1>(getManifestRes) != 200) {
        if (get<2>(getManifestRes))
            loggerInstance()->error("Fetch image manifest from registry:", regAddr, "failed");
        else
            loggerInstance()->info("Image:", imgName + ":" + tag, "not found");
        return -1;
    }
    string manifestRaw = get<0>(getManifestRes);

    // parse manifest
    int manifestSchemaVersion = 0;
    ImageData imageData;
    try {
        manifestSchemaVersion = imageData.buildFromRaw(manifestRaw);
        if (manifestSchemaVersion != 1 && manifestSchemaVersion != 2) {
            loggerInstance()->error("Unknown manifest version:", manifestSchemaVersion);
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
        const string filePath = layerDirPath + blobSum.substr(7) + ".tar.gz";
        const string dstDirPath = overlayDirPath + blobSum.substr(7) + "/";
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
    if (manifestSchemaVersion == 2) {
        // get config directly
        client.set_follow_location(true);
        auto fetchConfigRes = fetchV2Config(client, imageData, imgName, tag);
        if (fetchConfigRes.first != 0)
            return -1;
        imageData.config = fetchConfigRes.second;
    } else if (manifestSchemaVersion != 1) {
        loggerInstance()->error("Unknown manifest version");
        return -1;
    }

    loggerInstance()->info("Storing image");
    // make image dir
    const string confDigNoPrefix = imageData.configBlobDigest.substr(7);
    int ret = mkdir((imageDirPath + confDigNoPrefix).c_str(), S_IRWXU | S_IRWXG | S_IROTH);
    if (ret == -1) {
        loggerInstance()->sysError(errno, "Make image dir:", imageDirPath + confDigNoPrefix,
                                   "failed");
        return -1;
    }
    // store image manifest
    try {
        const string formattedManifest = imageData.manifest.dump(4);
        ofstream manifestOFStream(imageDirPath + confDigNoPrefix + "/manifest.json", std::ios::out);
        manifestOFStream.write(formattedManifest.c_str(), formattedManifest.size());
    } catch (const std::exception &e) {
        loggerInstance()->error("Write manifest failed:", e.what());
        return -1;
    }
    // store image config
    try {
        const string formateedConfig = imageData.config.dump(4);
        ofstream imgConfigOFStream(imageDirPath + confDigNoPrefix + "/config.json", std::ios::out);
        imgConfigOFStream.write(formateedConfig.c_str(), formateedConfig.size());
    } catch (const std::exception &e) {
        loggerInstance()->error("Write image configuration failed:", e.what());
        return -1;
    }
    // store image info
    repoInfo.addImage(imgName, tag, imageData.configBlobDigest);
    try {
        std::ofstream repoFStream(imageDirPath + imageRepoFileName);
        repoFStream << repoInfo.toJsonString();
        repoFStream.close();
    } catch (const std::exception &e) {
        loggerInstance()->error("Write image repo info failed:", e.what());
    }

    loggerInstance()->info("Image", imgName + ":" + tag,
                           "stored at:", imageDirPath + confDigNoPrefix);
    return 0;
}

#endif // PULL_CPP
