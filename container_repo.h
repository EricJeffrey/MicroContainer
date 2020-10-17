#if !defined(CONTAINER_REPO_H)
#define CONTAINER_REPO_H

#include "container_repo_item.h"
#include "repo.h"

#include <ostream>

/**
 * @brief contianer database manager, call [open] to open db and do [add/remove/update]
 */
struct ContainerRepo : public Repo {
    ContainerRepo() {}
    ~ContainerRepo() {}

    ContainerRepoItem getItem(const string &contId) { return ContainerRepoItem(this->get(contId)); }
    void updateStatus(const string &contId, ContainerStatus contStatus, time_t time = 0);
    bool isImageUsed(const string &imageId);
};

std::ostream &operator<<(std::ostream &out, const ContainerRepo &repo);

#endif // CONTAINER_REPO_H
