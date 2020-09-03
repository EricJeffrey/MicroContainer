#if !defined(PULL_H)
#define PULL_H

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "logger.h"
#include "httplib.h"
#include "json.hpp"

#include <openssl/sha.h>
#include <sstream>
#include <ostream>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <cstdio>
#include <vector>

const string imageDirPath = "./build/images/";
const string layerDirPath = "./build/layers/";

const string defaultRegAddr = "https://ejlzjv4p.mirror.aliyuncs.com";
// const string defaultRegAddr = "https://docker.mirrors.ustc.edu.cn";
const time_t readTimeoutInSec = 30;

enum RegistryEndPoint { CHECK = 0, IMAGE_MANIFESTS, IMAGE_BLOBS };
class RegistryHelper {
public:
    static string getPath(RegistryEndPoint endPoint, std::vector<string> args = {}) {
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
};

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

int pull(const string &imgNameTag, const string &regAddr = defaultRegAddr);
int pull(const string &imgName, const string &tag, const string &regAddr);

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
    loggerInstance()->info({"Fetching image", imgName + ":" + tag});
    httplib::Client client(regAddr.c_str());
    client.set_read_timeout(readTimeoutInSec);

    loggerInstance()->info({"Connecting to registry:", regAddr});
    // check connection
    httplib::Result resp = client.Get(RegistryHelper::getPath(RegistryEndPoint::CHECK).c_str());
    if (resp.error() || resp->status != 200) {
        if (resp.error())
            loggerInstance()->error({"Connect to registry:", regAddr, "failed"});
        else
            loggerInstance()->error(
                {"Cannot fetch image from registry:", regAddr, "v2 not supported"});
        return -1;
    }

    loggerInstance()->info("Fetching image manifest");
    // get manifest
    httplib::Headers headers(
        {std::make_pair("Accept", "application/vnd.docker.distribution.manifest.v2+json")});
    resp = client.Get(
        RegistryHelper::getPath(RegistryEndPoint::IMAGE_MANIFESTS, {imgName, tag}).c_str(),
        headers);
    if (resp.error() || resp->status != 200) {
        if (resp.error())
            loggerInstance()->error({"Fetch image manifest from registry:", regAddr, "failed"});
        else
            loggerInstance()->info({"Image:", imgName + ":" + tag, "not found"});
        return -1;
    }
    // parse manifest
    string manifest = resp->body;
    nlohmann::json manifestJson;
    std::set<string> blobSumSet;
    int manifestSchemaVersion;
    string configBlobDigest;
    nlohmann::json imageConfig;
    try {
        manifestJson = nlohmann::json::parse(manifest);
        manifestSchemaVersion = manifestJson["schemaVersion"].get<int>();

        if (manifestSchemaVersion == 2) {
            configBlobDigest = manifestJson["config"]["digest"].get<string>();
            for (auto &&blob : manifestJson["layers"])
                blobSumSet.insert(blob["digest"].get<string>());
        } else if (manifestSchemaVersion == 1) {
            // just extract config in .history
            const string imgConfig = manifestJson["history"][0]["v1Compatibility"].get<string>();
            configBlobDigest =
                string("sha256:") + sha256_string(imgConfig.c_str(), imgConfig.size());
            imageConfig = nlohmann::json::parse(imgConfig);

            for (auto &&blob : manifestJson["fsLayers"])
                blobSumSet.insert(blob["blobSum"].get<string>());
        } else {
            loggerInstance()->error(
                {"Unknown manifest version:", std::to_string(manifestSchemaVersion)});
            return -1;
        }
    } catch (const std::exception &e) {
        loggerInstance()->error({"Parsing manifest failed:", e.what()});
        return -1;
    }

    // fetch blobs
    loggerInstance()->info("Fetching image layers");
    client.set_follow_location(true);
    std::cerr << std::fixed << std::setprecision(2);
    try {
        for (auto &&blobSum : blobSumSet) {
            size_t totalSize = 0, receivedSize = 0;
            const string blobAddr =
                RegistryHelper::getPath(RegistryEndPoint::IMAGE_BLOBS, {imgName, blobSum});
            ofstream layerFStream(layerDirPath + blobSum.substr(7) + ".tar.gz", std::ios::out);
            httplib::Result blobResp = client.Get(
                blobAddr.c_str(), httplib::Headers(),
                [&](const httplib::Response &tmpResp) {
                    if (tmpResp.status != 200 && tmpResp.status / 100 != 3) {
                        loggerInstance()->error(
                            {"Fetch image layer failed, invalid response code"});
                        return false;
                    }
                    totalSize = std::stoul(tmpResp.headers.find("Content-Length")->second);
                    return true;
                },
                [&](const char *data, size_t dataLength) {
                    layerFStream.write(data, dataLength);
                    receivedSize += dataLength;
                    // todo change to logger::raw, use function template
                    std::cerr << "\r" << blobSum.substr(7, 16) << ": "
                              << receivedSize * 1.0 / totalSize * 100 << "%";
                    return true;
                });
            layerFStream.flush();
            layerFStream.close();
            std::cerr << endl;
        }
    } catch (const std::exception &e) {
        loggerInstance()->error({"Fetch image layer failed:", e.what()});
        return -1;
    }

    // fetch config
    loggerInstance()->info("Fetching image configuration");
    if (manifestSchemaVersion == 2) {
        // get config directly
        client.set_follow_location(true);
        auto imgConfigBlobResp = client.Get(
            RegistryHelper::getPath(RegistryEndPoint::IMAGE_BLOBS, {imgName, configBlobDigest})
                .c_str());
        if (imgConfigBlobResp.error()) {
            loggerInstance()->error({"Get image configuration failed:", imgConfigBlobResp.error()});
            return -1;
        }
        if (imgConfigBlobResp->status == 200) {
            try {
                imageConfig = nlohmann::json::parse(imgConfigBlobResp->body);
            } catch (const std::exception &e) {
                loggerInstance()->error({"Parsing v2 configuration failed:", e.what()});
                return -1;
            }
        } else if (imgConfigBlobResp->status == 404) {
            // fall back to v1
            auto imgV1ManifestResp = client.Get(
                RegistryHelper::getPath(RegistryEndPoint::IMAGE_MANIFESTS, {imgName, tag}).c_str(),
                httplib::Headers({std::make_pair(
                    "Accept", "application/vnd.docker.distribution.manifest.v1+json")}));
            if (imgV1ManifestResp.error()) {
                loggerInstance()->error(
                    {"Fetch image v1 manifest failed:", imgV1ManifestResp.error()});
                return -1;
            }
            if (imgV1ManifestResp->status == 200) {
                try {
                    imageConfig = nlohmann::json::parse(
                        imgV1ManifestResp->body)["history"][0]["v1Compatibility"];
                } catch (const std::exception &e) {
                    loggerInstance()->error({"Parsing v1 manifest failed:", e.what()});
                    return -1;
                }
            } else {
                loggerInstance()->error({"Fetch image v1 manifst failed, invalid response code:",
                                         std::to_string(imgV1ManifestResp->status)});
                return -1;
            }
        }
    } else {
        loggerInstance()->error("Unknown manifest version");
        return -1;
    }

    loggerInstance()->info("Storing image");
    // store image manifest
    const string confDigNoPrefix = configBlobDigest.substr(7);
    int ret = mkdir((imageDirPath + confDigNoPrefix).c_str(), S_IRWXU | S_IRWXG | S_IROTH);
    if (ret == -1) {
        loggerInstance()->sysError(errno,
                                   "Make image dir: " + imageDirPath + confDigNoPrefix + " failed");
        return -1;
    }
    try {
        const string formattedManifest = manifestJson.dump(4);
        ofstream manifestOFStream(imageDirPath + confDigNoPrefix + "/manifest.json", std::ios::out);
        manifestOFStream.write(formattedManifest.c_str(), formattedManifest.size());
    } catch (const std::exception &e) {
        loggerInstance()->error({"Write manifest failed:", e.what()});
        return -1;
    }
    // store image config
    try {
        const string formateedConfig = imageConfig.dump(4);
        ofstream imgConfigOFStream(imageDirPath + confDigNoPrefix + "/config.json", std::ios::out);
        imgConfigOFStream.write(formateedConfig.c_str(), formateedConfig.size());
    } catch (const std::exception &e) {
        loggerInstance()->error({"Write image configuration failed:", e.what()});
        return -1;
    }

    loggerInstance()->info(
        {"Image", imgName + ":" + tag, "stored at:", imageDirPath + confDigNoPrefix});
    return 0;
}

#endif // PULL_H
