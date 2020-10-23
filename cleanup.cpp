#if !defined(CLEANUP_CPP)
#define CLEANUP_CPP

#include "cleanup.h"
#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"
#include "network.h"

#include <sys/mount.h>

// todo return ip address on cleanup
void cleanup(const string &cont) noexcept {
    // update repo db
    try {
        string id;
        {
            ContainerRepo repo;
            repo.open(CONTAINER_REPO_DB_PATH());
            if (repo.contains(cont))
                id = cont;
            else {
                repo.foreach ([&id, &cont](int i, const string &k, const string &v) {
                    auto item = ContainerRepoItem(v);
                    if (item.containerID.substr(0, cont.size()) == cont || item.name == cont) {
                        id = item.containerID;
                        return true;
                    }
                    return false;
                });
            }
            if (!id.empty())
                repo.updateStatus(id, ContainerStatus::STOPPED, now());
            repo.close();
        }
        if (id.empty()) {
            loggerInstance()->info("Container", cont, "does not exist");
            return;
        }
        //
        if (umount((CONTAINER_DIR_PATH() + id + "/merged").c_str()) == -1) {
            const int err = errno;
            if (err != EINVAL && err != ENOENT)
                loggerInstance()->sysError(err, "Call to umount failed:");
        };
        cleanupContNet(id);

    } catch (std::exception &e) {
        loggerInstance()->error("cleanup failed while updating repo:", e.what());
    }
}

#endif // CLEANUP_CPP
