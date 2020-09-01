#if !defined(PULL_H)
#define PULL_H

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "logger.h"
#include "httplib.h"
#include "json.hpp"

#include <sstream>
#include <ostream>
#include <set>
#include <map>
#include <string>
#include <cstdio>

typedef const string &constrref;

const string registryAddr = "https://ejlzjv4p.mirror.aliyuncs.com";

void checkAndThrow(const httplib::Result &resp, const string &msg) {
    using std::runtime_error;
    if (resp.error()) {
        loggerInstance()->error({msg, to_string(resp.error())});
        throw runtime_error("http request failed");
    } else if (resp->status != 200) {
        loggerInstance()->error({msg, to_string(resp->status)});
        throw runtime_error("http resposne invalid");
    }
}

int pull(constrref imgName, constrref tag) {
    httplib::Client client(registryAddr.c_str());
    // check registry version
    client.set_read_timeout(30);
    auto resp = client.Get("/v2/");
    checkAndThrow(resp, "Get /v2/ failed");
    // get manifest
    loggerInstance()->debug({"fetching image:" , imgName, ":", tag, "from", registryAddr});
    httplib::Headers header;
    header.insert(std::make_pair("Accept", "application/vnd.docker.distribution.manifest.v1+json"));
    resp = client.Get(("/v2/library/" + imgName + "/manifests/" + tag).c_str());
    checkAndThrow(resp, "GET /v2/.../manifests/... failed");
    loggerInstance()->debug("manifest fetched, pulling layers");
    // store manifest
    nlohmann::json respJson = nlohmann::json::parse(resp->body);
    std::set<string> blobSet;
    for (auto &&blob : respJson["fsLayers"]) {
        const string tmpBlobSum = blob["blobSum"].dump();
        blobSet.insert(tmpBlobSum.substr(1, tmpBlobSum.size() - 2));
    }

    std::cerr << std::fixed << std::setprecision(2);
    try {
        for (auto &&blobSum : blobSet) {
            size_t totalSize = 0, curSize = 0;
            const string blobAddr = "/v2/library/" + imgName + "/blobs/" + blobSum;
            ofstream fileOStream("build/layers/" + blobSum.substr(7) + ".tar.gz", std::ios::out);
            client.set_follow_location(true);
            auto blobResp = client.Get(
                (blobAddr).c_str(), httplib::Headers(),
                [&](const httplib::Response &curResp) {
                    if (curResp.status != 200 && curResp.status / 100 != 3) {
                        loggerInstance()->error({"无效的http响应码: ", to_string(curResp.status)});
                        throw std::runtime_error("http resposne status invalid");
                    }
                    totalSize = std::stoi(curResp.headers.find("Content-Length")->second);
                    return true;
                },
                [&](const char *data, size_t dataLength) {
                    fileOStream.write(data, dataLength);
                    curSize += dataLength;
                    std::cerr << "\r" << blobSum.substr(7, 16) << ": "
                              << curSize * 1.0 / totalSize * 100 << "%";
                    return true;
                });
            fileOStream.close();
            std::cerr << endl;
        }
    } catch (const std::exception &e) {
        loggerInstance()->error({"镜像获取失败", e.what()});
    }

    return 0;
}

#endif // PULL_H
