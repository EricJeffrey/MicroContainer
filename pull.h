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

// string sha256_string(const char *str, size_t len) {
//     char output_buffer[65];
//     std::fill(output_buffer, output_buffer + 65, 0);
//     unsigned char hash[SHA256_DIGEST_LENGTH];
//     SHA256_CTX sha256;
//     SHA256_Init(&sha256);
//     SHA256_Update(&sha256, str, len);
//     SHA256_Final(hash, &sha256);
//     int i = 0;
//     for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
//         sprintf(output_buffer + (i * 2), "%02x", hash[i]);
//     }
//     return string(output_buffer);
// }

int pull(const string &imgNameTag, const string &regAddr = defaultRegAddr);
int pull(const string &imgName, const string &tag, const string &regAddr);

int pull(const string &imgNameTag, const string &regAddr) {
    string imgName, tag;
    const size_t colonPos = imgNameTag.find(':');
    if (colonPos != imgNameTag.size()) {
        imgName = imgNameTag.substr(0, colonPos);
        tag = imgNameTag.substr(colonPos + 1);
    } else {
        imgName = imgNameTag, tag = "latest";
    }
    return pull(imgName, tag, regAddr);
}

// No throw
int pull(const string &imgName, const string &tag, const string &regAddr) {
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
    string manifestSha256;
    nlohmann::json manifestJson;
    std::set<string> blobSumSet;
    try {
        // manifestSha256 = sha256_string(manifest.c_str(), manifest.size());
        manifestSha256 = resp->headers.find("Docker-Content-Digest")->second.substr(7);
        manifestJson = nlohmann::json::parse(manifest);
        for (auto &&blob : manifestJson["layers"])
            blobSumSet.insert(blob["digest"].get<string>());
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

    loggerInstance()->info("Storing image");
    // store image
    int ret = mkdir((imageDirPath + manifestSha256).c_str(), S_IRWXU | S_IRWXG | S_IROTH);
    if (ret == -1) {
        loggerInstance()->sysError(errno,
                                   "Make image dir: " + imageDirPath + manifestSha256 + " failed");
        return -1;
    }
    try {
        const string formattedManifest = manifestJson.dump(4);
        ofstream manifestOFStream(imageDirPath + manifestSha256 + "/manifest", std::ios::out);
        manifestOFStream.write(formattedManifest.c_str(), formattedManifest.size());
    } catch (const std::exception &e) {
        loggerInstance()->error({"Write manifest failed:", e.what()});
        return -1;
    }
    loggerInstance()->info(
        {"Image", imgName + ":" + tag, "stored at:", imageDirPath + manifestSha256});
    return 0;
}

#endif // PULL_H
