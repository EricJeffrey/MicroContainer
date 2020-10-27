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
            ContainerRepoItem contItem;
            if (auto targetItem = containerExist(cont); targetItem.has_value()) {
                contItem = targetItem.value();
            } else {
                loggerInstance()->info("container", cont, "not found");
                return;
            }
            if (contItem.status.contStatus == ContainerStatus::RUNNING) {
                loggerInstance()->info("container is still running, please stop it first");
                return;
            } else if (contItem.status.contStatus == ContainerStatus::INVALID) {
                loggerInstance()->info("invalid container status, you may need to recreate it");
                return;
            } else if (contItem.status.contStatus != ContainerStatus::STOPPED) {
                // only need to cleanup stopped container
                return;
            }
            id = contItem.containerID;
        }
        if (id.empty()) {
            loggerInstance()->info("Container", cont, "does not exist");
            return;
        }
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
