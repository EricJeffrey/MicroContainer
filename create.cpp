#if !defined(CREATE_CPP)
#define CREATE_CPP

#include "network.h"
#include "create.h"
#include "logger.h"
#include "utils.h"
#include "config.h"
#include "container_repo.h"
#include "image_repo.h"

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
    fork_exec_wait(ContainerRtPath, {ContainerRtName, "spec"}, true);
}

// return <bool, str>: <exist, id_without_sha>
PairBoolStr checkImgExist(const string &imgName) {
    auto imgRepo = ImageRepo::fromFile(ImageDirPath + ImageRepoFileName);
    size_t pos = imgName.find(':');
    if (pos != imgName.npos) {
        const string name = imgName.substr(0, pos), tag = imgName.substr(pos + 1);
        if (imgRepo.contains(name, tag))
            return PairBoolStr(true, imgRepo.getId(name, tag));
    } else {
        if (imgRepo.contains(imgName))
            return PairBoolStr(true, imgRepo.getId(imgName));
        if (imgRepo.contains(imgName, "latest"))
            return PairBoolStr(true, imgRepo.getId(imgName, "latest"));
    }
    return PairBoolStr(false, "");
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
    if (mkdir((ContainerDirPath + id).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        loggerInstance()->sysError(errno, "Call to mkdir:", ContainerDirPath + id, "failed");
        throw runtime_error("call to mkdir failed");
    }
    // make dirs
    vector<string> dirNames({"merged", "work", "diff"});
    for (auto &&name : dirNames) {
        if (mkdir((ContainerDirPath + id + "/" + name).c_str(),
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
            loggerInstance()->sysError(errno, "Call to mkdir:", ContainerDirPath + id + "/" + name,
                                       "failed");
            throw runtime_error("call to mkdir failed");
        }
    }
    // make lower file
    ofstream lowerOFS(ContainerDirPath + id + "/lower");
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
    nlohmann::json contSpecConfig = ContainerRepo::defaultSpecConfig;
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
    contSpecConfig["root"]["path"] = ContainerDirPath + id + "/merged";
    // netns
    for (auto &&ns : contSpecConfig["linux"]["namespaces"]) {
        if (ns["type"].get<string>() == "network") {
            ns["path"] = netNsPathPrefix + id;
            break;
        }
    }
    return contSpecConfig;
}

// write repo out to containers/repo.json
void writeOutRepo(const ContainerRepo &repo) {
    std::ofstream ofstream(ContainerDirPath + ContainerRepoFileName);
    auto content = repo.toJsonStr();
    ofstream.write(content.c_str(), content.size());
}

void createContainer(const string &name, const string &imgName) noexcept {
    try {
        // check if container repo exist
        if (!isRegFileExist(ContainerDirPath + ContainerRepoFileName))
            createFile(ContainerDirPath + ContainerRepoFileName, "{}");
        const string containerId = sha256_string(name.c_str(), name.size());
        auto repo = ContainerRepo::fromFile(ContainerDirPath + ContainerRepoFileName);
        if (repo.contains(containerId)) {
            loggerInstance()->info("container:", name, "exists");
            return;
        }
        string imgId;
        {
            auto tmpRes = checkImgExist(imgName);
            if (!tmpRes.first) {
                loggerInstance()->info("can not find image:", imgName);
                return;
            }
            imgId = tmpRes.second;
        }
        auto layers = getImgLayers(imgId);
        auto imgContConfig = getImgContConfig(imgId);
        auto contSpecConfig = mkContSpecConfig(containerId, imgContConfig);
        prepareContDirs(containerId, layers);
        createFile(ContainerDirPath + containerId + "/config.json", contSpecConfig.dump(4));
        // update repo info {id: {name, imageid, state:created, createdTime:now}}
        repo.add(containerId, name, imgId, ContState::created, nowISO());
        writeOutRepo(repo);
        std::cerr << containerId << std::endl;
    } catch (const std::exception &e) {
        loggerInstance()->error("Create container failed:", e.what());
    } catch (...) {
        loggerInstance()->error("Create container failed");
    }
}

#endif // CREATE_CPP
