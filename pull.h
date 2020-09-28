#if !defined(PULL_H)
#define PULL_H

#include "logger.h"
#include "httplib.h"
#include "json.hpp"
#include "config.h"
#include "utils.h"

#include <sstream>
#include <ostream>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <cstdio>
#include <vector>

// TODO should use a specified output stream

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
int pull(const string &imgNameTag, const string &regAddr = DefaultRegAddr) noexcept;

// no throw
int pull(const string &imgName, const string &tag, const string &regAddr)noexcept;

#endif // PULL_H
