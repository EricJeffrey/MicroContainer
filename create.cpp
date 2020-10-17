#if !defined(CREATE_CPP)
#define CREATE_CPP

#include "create.h"
#include "config.h"
#include "container_repo.h"
#include "image_repo.h"
#include "lib/logger.h"
#include "network.h"
#include "utils.h"

#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef std::pair<bool, string> PairBoolStr;

// generate OCI-runtime config.json
void generateSpecConf(const string &dirPath) {
    if (chdir(dirPath.c_str()) == -1) {
        loggerInstance()->sysError(errno, "Call to chdir:", dirPath, "failed");
        throw runtime_error("call to chdir failed");
    }
    fork_exec_wait(CONTAINER_RT_PATH, {CONTAINER_RT_NAME, "spec"}, true);
}

// return <bool, str>: <exist, id_without_sha>
std::optional<string> checkImgExist(const string &imgName) {

    ImageRepo imgRepo;
    imgRepo.open(IMAGE_REPO_DB_PATH);
    size_t pos = imgName.find(':');
    if (pos != imgName.npos) {
        const string name = imgName.substr(0, pos), tag = imgName.substr(pos + 1);
        if (imgRepo.contains(name, tag))
            return imgRepo.getItem(name, tag).imageID;
    } else {
        if (imgRepo.contains(imgName))
            return imgRepo.getItem(imgName).imageID;
        if (imgRepo.contains(imgName, "latest"))
            return imgRepo.getItem(imgName, "latest").imageID;
    }
    return {};
}

// get layer ids of image
vector<string> getImgLayers(const string &imgId) {
    auto imgManifest = getImgManifest(imgId);
    auto &layers = imgManifest["layers"];
    vector<string> resLayers;
    for (auto &&layer : layers) {
        resLayers.push_back(layer["digest"].get<string>().substr(7));
    }
    return resLayers;
}

// make id/[merged, work, diff] id/lower
void prepareContDirs(const string &id, const vector<string> &layers) {
    if (mkdir((CONTAINER_DIR_PATH + id).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        loggerInstance()->sysError(errno, "Call to mkdir:", CONTAINER_DIR_PATH + id, "failed");
        throw runtime_error("call to mkdir failed");
    }
    // make dirs
    vector<string> dirNames({"merged", "work", "diff"});
    for (auto &&name : dirNames) {
        if (mkdir((CONTAINER_DIR_PATH + id + "/" + name).c_str(),
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
            loggerInstance()->sysError(errno, "Call to mkdir:", CONTAINER_DIR_PATH + id + "/" + name,
                                       "failed");
            throw runtime_error("call to mkdir failed");
        }
    }
    // make lower file
    ofstream lowerOFS(CONTAINER_DIR_PATH + id + "/lower");
    string ss;
    for (size_t i = 0; i < layers.size(); i++) {
        if (i != 0)
            ss += ":";
        ss += layers[i];
    }
    // layer1:layer2:layer3...
    lowerOFS.write(ss.c_str(), ss.size());
}

// using default spec config and update `env, args, hostname, rootfs.path, netns`
nlohmann::json mkContSpecConfig(const string &id, const nlohmann::json &imgContConf) {
    nlohmann::json contSpecConfig = DEFAULT_CONTAINER_CONF_SPEC;
    // env
    contSpecConfig["process"]["env"] = imgContConf["Env"];
    // args
    nlohmann::json args = nlohmann::json::array();
    for (auto &&entry : imgContConf["Entrypoint"])
        args.push_back(entry);
    for (auto &&cmd : imgContConf["Cmd"])
        args.push_back(cmd);
    contSpecConfig["process"]["args"] = args;
    // hostname
    contSpecConfig["hostname"] = id.substr(0, 12);
    // rootfs.path
    contSpecConfig["root"]["path"] = CONTAINER_DIR_PATH + id + "/merged";
    // netns
    for (auto &&ns : contSpecConfig["linux"]["namespaces"]) {
        if (ns["type"].get<string>() == "network") {
            ns["path"] = netNsPathPrefix + id;
            break;
        }
    }
    return contSpecConfig;
}

void createContainer(const string &imgName, const string &name) noexcept {
    try {
        const string containerId = sha256_string(name.c_str(), name.size());
        // check local existence
        {
            ContainerRepo repo;
            repo.open(CONTAINER_REPO_DB_PATH);
            if (repo.contains(containerId)) {
                loggerInstance()->info("container:", name, "exists");
                return;
            }
        }
        string imgId;
        {
            auto tmpRes = checkImgExist(imgName);
            if (!tmpRes.has_value()) {
                loggerInstance()->info("can not find image:", imgName);
                return;
            }
            imgId = tmpRes.value();
        }
        auto contSpecConfig = mkContSpecConfig(containerId, getImgContConfig(imgId));
        prepareContDirs(containerId, getImgLayers(imgId));
        createFile(CONTAINER_DIR_PATH + containerId + "/config.json", contSpecConfig.dump(4));
        // update repo info {id: {name, imageid, state:created, createdTime:now}}
        {
            ContainerRepo repo;
            repo.open(CONTAINER_REPO_DB_PATH);
            ContainerRepoItem item(containerId, imgId, name,
                                   concat(contSpecConfig["process"]["args"]), now(),
                                   ContainerRepoItem::Status(CREATED, now()));
            repo.add(containerId, item);
        }
        std::cerr << containerId << std::endl;
    } catch (const std::exception &e) {
        loggerInstance()->error("Create container failed:", e.what());
    } catch (...) {
        loggerInstance()->error("Create container failed");
    }
}

#endif // CREATE_CPP
