#if !defined(IMAGE_REPO)
#define IMAGE_REPO

#include "lib/json.hpp"

#include <unordered_map>
#include <string>

using std::string;
using std::unordered_map;

class ImageRepoInfo {
private:
    unordered_map<string, unordered_map<string, string>> repository;

public:
    ImageRepoInfo() {}
    ~ImageRepoInfo() {}

    inline void addImage(const string &name, const string &tag, const string &sha) {
        const string &nameWithTag = name + ":" + tag;
        if (repository.find(name) == repository.end() ||
            repository[name].find(nameWithTag) == repository[name].end())
            repository[name][nameWithTag] = sha;
    }

    inline bool contains(const string &name, const string &tag) {
        return repository.find(name) != repository.end() &&
               repository[name].find(name + ":" + tag) != repository[name].end();
    }

    // throw
    inline string toJsonString() { return nlohmann::json(repository).dump(4); }

    // throw
    static ImageRepoInfo buildFromJson(const nlohmann::json &repoJson) {
        ImageRepoInfo resRepoInfo;
        for (auto &&imageTagJson : repoJson.items()) {
            for (auto &&tagJson : imageTagJson.value().items()) {
                resRepoInfo.repository[imageTagJson.key()][tagJson.key()] =
                    tagJson.value().get<string>();
            }
        }
        return resRepoInfo;
    }
};

#endif // IMAGE_REPO
