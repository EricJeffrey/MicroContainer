#if !defined(IMAGE_REPO)
#define IMAGE_REPO

#include "json.hpp"

#include <unordered_map>
#include <string>
#include <fstream>
#include <istream>

using std::string;
using std::unordered_map;

typedef unordered_map<string, unordered_map<string, string>> ImgRepoContent;

class ImageRepo {
private:
    ImgRepoContent repo;

public:
    ImageRepo() {}
    explicit ImageRepo(const ImgRepoContent &repo) : repo(repo) {}
    ~ImageRepo() {}

    void add(const string &name, const string &tag, const string &sha) {
        const string &nameWithTag = name + ":" + tag;
        if (repo.find(name) == repo.end() || repo[name].find(nameWithTag) == repo[name].end())
            repo[name][nameWithTag] = sha;
    }

    // return empty string when not found, use [contains] to check first
    string getId(const string &shortId) {
        for (auto &&imgRepo : repo) {
            for (auto &&shaId : imgRepo.second) {
                if (shaId.second.substr(7, shortId.size()) == shortId)
                    return shaId.second.substr(7);
            }
        }
        return "";
    }

    string getId(const string &name, const string &tag) {
        return repo[name][name + ":" + tag].substr(7);
    }

    bool contains(const string &name, const string &tag) {
        return repo.find(name) != repo.end() &&
               repo[name].find(name + ":" + tag) != repo[name].end();
    }

    // id should not start with 'sha256:'
    bool contains(const string &id) { return getId(id).size() != 0; }

    // throw
    string toJsonString() { return nlohmann::json(repo).dump(4); }

    static ImageRepo fromFile(const string &filePath) {
        std::ifstream imgRepoFStream(filePath);
        return buildFromJson(nlohmann::json::parse(string(
            (std::istreambuf_iterator<char>(imgRepoFStream)), std::istreambuf_iterator<char>())));
    }

    // throw
    static ImageRepo buildFromJson(const nlohmann::json &repoJson) {
        return ImageRepo(repoJson.get<ImgRepoContent>());
    }
};

inline nlohmann::json getImgManifest(const string &imgId) {
    std::ifstream imgManiFS(imageDirPath + imgId + "/manifest.json");
    return nlohmann::json::parse(
        string((std::istreambuf_iterator<char>(imgManiFS)), std::istreambuf_iterator<char>()));
}

inline nlohmann::json getImgContConfig(const string &imgId) {
    std::ifstream imgConfig(imageDirPath + imgId + "/config.json");
    return nlohmann::json::parse(string((std::istreambuf_iterator<char>(imgConfig)),
                                        std::istreambuf_iterator<char>()))
        .at("container_config");
}

#endif // IMAGE_REPO
