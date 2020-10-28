#if !defined(CLEANUP_CPP)
#define CLEANUP_CPP

#include "cleanup.h"
#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"
#include "network.h"
#include "stop.h"

#include <filesystem>
#include <sys/mount.h>

// todo return ip address on cleanup
void cleanup(const string &cont, bool force) noexcept {
    // update repo db
    try {
        string containerId;
        {
            ContainerRepoItem contItem;
            if (auto targetItem = containerExist(cont); targetItem.has_value()) {
                contItem = targetItem.value();
            } else {
                loggerInstance()->info("container", cont, "not found");
                return;
            }
            // do not clean running container
            if (contItem.status.contStatus == ContainerStatus::RUNNING) {
                if (force) {
                    loggerInstance()->info("container running, stopping");
                    // force stop container
                    stop(contItem.containerID, true);
                } else {
                    loggerInstance()->info("container running");
                    return;
                }
            } else if (contItem.status.contStatus == ContainerStatus::INVALID) {
                loggerInstance()->info("invalid container status, you may need to recreate it");
                return;
            } else if (contItem.status.contStatus != ContainerStatus::STOPPED) {
                // only need to cleanup [CREATED] container
                return;
            }
            containerId = contItem.containerID;
        }
        // delete crun
        if (int tmp = fork_exec_wait(CONTAINER_RT_PATH, {CONTAINER_RT_NAME, "delete", containerId});
            tmp != 0)
            loggerInstance()->warn("failed to cleanup runtime container, return value:", tmp);
        if (containerId.empty()) {
            loggerInstance()->info("Container", cont, "does not exist");
            return;
        }
        if (umount((CONTAINER_DIR_PATH() + containerId + "/merged").c_str()) == -1) {
            const int err = errno;
            if (err != EINVAL && err != ENOENT)
                loggerInstance()->sysError(err, "Call to umount failed:");
        };
        cleanupContNet(containerId);
        // remove conmon files
        {
            vector<string> filenamesToRemove = {"winsz", "shm", "ctl", "attach"};
            for (auto &&filename : filenamesToRemove) {
                std::filesystem::remove(CONTAINER_DIR_PATH() + containerId + "/" + filename);
            }
        }

    } catch (std::exception &e) {
        loggerInstance()->error("cleanup failed:", e.what());
    } catch (...) {
        loggerInstance()->error("cleanup failed");
    }
}

#endif // CLEANUP_CPP
