#if !defined(IMAGE_BUILD_CPP)
#define IMAGE_BUILD_CPP

#include "image_build.h"
#include "image_repo.h"
#include "lib/json.hpp"
#include "lib/logger.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <istream>
#include <ostream>

// make iamge dirs
void createDirs(const string &imageId) {
    std::filesystem::create_directories(IMAGE_DIR_PATH() + imageId);
    std::filesystem::create_directories(IMAGE_DIR_PATH() + imageId + "/diff");
}

void removeDirs(const string &imageId) { std::filesystem::remove_all(IMAGE_DIR_PATH() + imageId); }

bool parseConfig(const string &filePath, const string &imageId, string &baseImageId,
                 nlohmann::json &config) {
    bool fromGot = false, cmdGot = false;
    std::ifstream fileStream{filePath};
    string line;
    while (std::getline(fileStream, line)) {
        trim(line);
        const int lineLength = line.size();
        if (lineLength > 0) {
            // parse here
            if (!fromGot) {
                if (lineLength > 5 && line.substr(0, 5) == "FROM ") {
                    // load container config
                    string baseImage = line.substr(5);
                    if (auto tmp = imageExist(baseImage); !tmp.has_value()) {
                        loggerInstance()->info("can not find base image:", baseImage);
                        return false;
                    } else {
                        baseImageId = tmp.value().imageID;
                        std::ifstream baseImgFileStream(IMAGE_DIR_PATH() + baseImageId +
                                                        "/config.json");
                        config = nlohmann::json::parse(
                            string{(std::istreambuf_iterator<char>(baseImgFileStream)),
                                   std::istreambuf_iterator<char>()})["container_config"];
                        fromGot = true;
                    }
                } else {
                    loggerInstance()->info("file should start with FROM");
                    return false;
                }
            } else {
                if (lineLength > 4 && line.substr(0, 4) == "CMD ") {
                    if (cmdGot) {
                        loggerInstance()->info("only one CMD is allowed");
                        return false;
                    } else {
                        // copy cmds to config
                        try {
                            auto cmdList = nlohmann::json::parse(line.substr(4));
                            if (cmdList.empty()) {
                                loggerInstance()->info("empty CMD is not allowed");
                                return false;
                            }
                            config["Cmd"] = cmdList;
                            cmdGot = true;
                        } catch (...) {
                            loggerInstance()->info("invalid CMD syntax");
                            return false;
                        }
                    }
                } else if (lineLength > 4 && line.substr(0, 4) == "ENV ") {
                    config["Env"].push_back(line.substr(4));
                } else if (lineLength > 5 && line.substr(0, 5) == "COPY ") {
                    auto posOfSpace = line.find_first_of(' ', 5);
                    if (posOfSpace == string::npos) {
                        loggerInstance()->info("invalid COPY syntax");
                        return false;
                    }
                    string srcPath = line.substr(5, posOfSpace - 5);
                    string dstPath = line.substr(posOfSpace + 1);
                    if (dstPath[0] != '/') {
                        loggerInstance()->info("destination of COPY should be absolute path");
                        return false;
                    }
                    dstPath = IMAGE_DIR_PATH() + imageId + "/diff" + dstPath;
                    std::filesystem::create_directories(
                        dstPath.substr(0, dstPath.find_last_of('/')));
                    std::filesystem::copy(srcPath, dstPath,
                                          std::filesystem::copy_options::recursive);
                } else {
                    loggerInstance()->info("unrecognized word");
                    return false;
                }
            }
        }
    }
    return true;
}

// todo verify functionality
void buildImage(const string &filePath, const string &name) noexcept {
    string imageId, imageName, imageTag = "latest";
    try {
        // get id
        {
            if (!std::filesystem::exists(filePath)) {
                loggerInstance()->info("please specify path of file to use");
                return;
            }
            imageName = name;
            if (imageName.empty())
                imageName = genRandomStr(8);
            const string &&nameTag = imageName + ":" + imageTag;
            imageId = sha256_string(nameTag.c_str(), nameTag.size());
        }

        // create diff dir
        createDirs(imageId);

        string baseImageId;
        nlohmann::json containerConfig;
        if (parseConfig(filePath, imageId, baseImageId, containerConfig)) {
            // load layers of base image
            nlohmann::json config = {{"created", nowISO()}};
            config["container_config"] = containerConfig;
            auto &&configStr = config.dump();
            auto configSize = configStr.size();
            // write manifest
            nlohmann::json manifest = {
                {"mediaType", "application/vnd.docker.distribution.manifest.v2+json"},
                {"schemaVersion", "2"},
            };
            manifest["config"] = {
                {"digest", "sha256:" + sha256_string(configStr.c_str(), configSize)},
                {"mediaType", "application/vnd.docker.container.image.v1+json"},
                {"size", std::to_string(configSize)},
            };
            nlohmann::json layers;
            {
                std::ifstream baseManiStream(IMAGE_DIR_PATH() + baseImageId + "/manifest.json");
                layers =
                    nlohmann::json::parse(string{(std::istreambuf_iterator<char>(baseManiStream)),
                                                 std::istreambuf_iterator<char>()})["layers"];
            }
            manifest["layers"] = layers;
            {
                // write manifest
                auto &&manifestStr = manifest.dump();
                std::ofstream manifestOStream(IMAGE_DIR_PATH() + imageId + "/manifest.json");
                manifestOStream.write(manifestStr.c_str(), manifestStr.size());
            }
            {
                // write config
                std::ofstream configOStream(IMAGE_DIR_PATH() + imageId + "/config.json");
                configOStream.write(configStr.c_str(), configSize);
            }
            // update repo
            {
                ImageRepo repo;
                repo.open(IMAGE_REPO_DB_PATH());
                ImageRepoItem tmpItem{"localhost", imageName, imageTag, imageId, now(), 0};
                repo.addImg(imageId, tmpItem);
                repo.close();
            }
            loggerInstance()->info("created", imageId);
        } else {
            removeDirs(imageId);
        }
        return;
    } catch (const std::exception &e) {
        loggerInstance()->error("build failed:", e.what());
    } catch (...) {
        loggerInstance()->error("build failed:");
    }
    if (!imageId.empty())
        removeDirs(imageId);
}

#endif // IMAGE_BUILD_CPP
