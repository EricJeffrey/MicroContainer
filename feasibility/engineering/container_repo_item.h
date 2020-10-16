#if !defined(CONTAINER_REPO_ITEM)
#define CONTAINER_REPO_ITEM

#include "repo_item.h"
#include "utils.h"

#include <ctime>
#include <string>
#include <vector>

using std::string, std::vector;

enum ContainerStatus { CREATED, RUNNING, STOPPED, INVALID };

struct ContainerRepoItem : public RepoItem {
    struct Status : public RepoItem {
        static const char SEP = '_';
        ContainerStatus type;
        time_t time;

        Status() {}
        Status(const string &str);
        Status &operator=(const Status &) = default;

        string toDBString() const override;
        string toString() const;
        void update(ContainerStatus contStatus, time_t time) {
            this->type = contStatus, this->time = time;
        }
    };
    static const char SEP = '|';
    static vector<string> ATTR_TAG_LIST;

    string containerID, imageID, name, command;
    time_t created;
    Status status;

    ContainerRepoItem(const string &str);
    string toDBString() const override;
    vector<string> toStringList();

    // extract image id of database-string
    static string extractImgID(const string &str);
};

#endif // CONTAINER_REPO_ITEM
