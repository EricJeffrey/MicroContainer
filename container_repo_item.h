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
        static constexpr char SEP = '_';
        ContainerStatus contStatus;
        time_t time;

        Status() {}
        Status(ContainerStatus contStatus, time_t time) : contStatus(contStatus), time(time) {}
        Status(const string &str);
        Status &operator=(const Status &) = default;

        string toDBString() const override;
        string toString() const;
        void update(ContainerStatus contStatus, time_t time) {
            this->contStatus = contStatus, this->time = time;
        }
    };
    static constexpr char SEP = '|';
    static vector<string> ATTR_TAG_LIST;

    string containerID, imageID, name, command;
    time_t created;
    Status status;

    ContainerRepoItem() {}
    ContainerRepoItem(const string &id, const string &imgId, const string &name, const string &cmd,
                      time_t created, Status status)
        : containerID(id), imageID(imgId), name(name), command(cmd), created(created),
          status(status) {}
    ContainerRepoItem(const string &str);

    string toDBString() const override;
    vector<string> toStringList();

    // extract image id of database-string
    static string extractImgID(const string &str);
};

#endif // CONTAINER_REPO_ITEM
