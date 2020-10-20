#if !defined(PULL_H)
#define PULL_H

#include "config.h"
#include "lib/httplib.h"
#include "lib/json.hpp"
#include "lib/logger.h"
#include "utils.h"

#include <cstdio>
#include <iostream>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

enum RegistryEndPoint { CHECK = 0, IMAGE_MANIFESTS, IMAGE_BLOBS };

struct ImageData {
    int schemaVersion;
    nlohmann::json manifest;
    nlohmann::json config;
    string confBlobDigest;
    std::set<string> layerBlobSumSet;

    // return schemaVersion, throw json parse error
    inline int buildFromRaw(const string &raw) {
        manifest = nlohmann::json::parse(raw);
        schemaVersion = manifest["schemaVersion"].get<int>();
        if (schemaVersion == 2) {
            confBlobDigest = manifest["config"]["digest"].get<string>();
            for (auto &&blob : manifest["layers"])
                layerBlobSumSet.insert(blob["digest"].get<string>());
        } else if (schemaVersion == 1) {
            string configRaw = manifest["history"][0]["v1Compatibility"].get<string>();
            confBlobDigest = "sha256:" + sha256_string(configRaw.c_str(), configRaw.size());
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
int pull(const string &imgNameTag, const string &regAddr = DEFAULT_REG_ADDR) noexcept;

#endif // PULL_H
