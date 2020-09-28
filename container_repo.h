#if !defined(CONTAINER_REPO_H)
#define CONTAINER_REPO_H

#include <unordered_map>
#include <string>
#include <istream>
#include <fstream>
#include <stdexcept>
#include <exception>
#include <vector>

#include "logger.h"
#include "json.hpp"

using std::string;
using std::unordered_map;
using std::vector;

static const vector<string> contState2Desc({"running", "stopped", "created"});

enum ContState { running = 0, stopped, created, maxval };

class ContInfoType {
public:
    string name, image;
    ContState state;
    string created;

    ContInfoType() {}
    ContInfoType(const string &name, const string &imgId, ContState state, const string &createT)
        : name(name), image(imgId), state(state), created(createT) {}
    ~ContInfoType() {}
};

typedef unordered_map<string, ContInfoType> ContRepoContent;

string toString(const ContState state);

ContState fromString(const string &state);

void to_json(nlohmann::json &j, const ContInfoType &containerInfo);
void from_json(const nlohmann::json &j, ContInfoType &containerInfo);

class ContainerRepo {
private:
    ContRepoContent repo;

public:
    static const nlohmann::json defaultSpecConfig;

    ContainerRepo() {}
    explicit ContainerRepo(ContRepoContent repoCont) : repo(repoCont) {}
    ~ContainerRepo() {}

    void add(const string &id, const string &name, const string &imgId, ContState state,
             const string createT) {
        repo[id] = ContInfoType(name, imgId, state, createT);
    }

    bool contains(const string &id) const { return repo.find(id) != repo.end(); }

    string toJsonStr() const { return nlohmann::json(repo).dump(4); }

    static ContainerRepo fromFile(const string &filePath);

    static nlohmann::json createSpecConfig(const vector<string> &envs, const vector<string> &args,
                                           const string &rootfsPath, const string &hostname,
                                           const string &netnsPath);
};

#endif // CONTAINER_REPO_H
