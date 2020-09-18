#if !defined(PULL_H)
#define PULL_H

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
const string imageRepoFileName = "repo.json";
const string layerDirPath = "./build/layers/";
const string overlayDirPath = "./build/overlay/";

const string defaultRegAddr = "https://ejlzjv4p.mirror.aliyuncs.com";
// const string defaultRegAddr = "https://docker.mirrors.ustc.edu.cn";
const time_t readTimeoutInSec = 30;

// FIXME should use a specified output stream

string sha256_string(const char *str, size_t len);

enum RegistryEndPoint { CHECK = 0, IMAGE_MANIFESTS, IMAGE_BLOBS };

struct ImageData {
    nlohmann::json manifest;
    int schemaVersion;
    nlohmann::json config;
    string configBlobDigest;
    std::set<string> layerBlobSumSet;

    // return schemaVersion, throw json parse error
    inline int buildFromRaw(const string &raw) {
        manifest = nlohmann::json::parse(raw);
        schemaVersion = manifest["schemaVersion"].get<int>();

        if (schemaVersion == 2) {
            configBlobDigest = manifest["config"]["digest"].get<string>();
            for (auto &&blob : manifest["layers"])
                layerBlobSumSet.insert(blob["digest"].get<string>());
        } else if (schemaVersion == 1) {
            string configRaw = manifest["history"][0]["v1Compatibility"].get<string>();
            configBlobDigest =
                string("sha256:") + sha256_string(configRaw.c_str(), configRaw.size());
            config = configRaw;
            for (auto &&blob : manifest["fsLayers"]) {
                layerBlobSumSet.insert(blob["blobSum"].get<string>());
            }
        }
        return schemaVersion;
    }
};

string getRegistryPath(RegistryEndPoint endPoint, std::vector<string> args = {});

// no throw
int pull(const string &imgNameTag, const string &regAddr = defaultRegAddr);

// no throw
int pull(const string &imgName, const string &tag, const string &regAddr);

typedef std::pair<int, bool> PairIntBool;
// check connection, return <http-status-code, client.error()>
PairIntBool checkV2Conn(httplib::Client &client);

typedef std::tuple<string, int, bool> TupStrIntBool;
// return <manifest, status-code, error>
TupStrIntBool getManifest(httplib::Client &client, const string &imgName, const string &tag);

// fetch & store blobs, throw httplib.client error
void fetchBlobs(httplib::Client &client, const ImageData &imageData, const string &imgName);

typedef std::pair<int, nlohmann::json> PairIntJson;
// return <errCode, configJson>, log on process
PairIntJson fetchV2Config(httplib::Client &client, const ImageData &imageData,
                          const string &imgName, const string &tag);

#endif // PULL_H
