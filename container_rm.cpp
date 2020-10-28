#if !defined(CONTAINER_RM_CPP)
#define CONTAINER_RM_CPP

#include "container_rm.h"
#include "config.h"
#include "container_repo.h"
#include "image_repo.h"
#include "lib/logger.h"

#include <filesystem>

void removeContainer(const string &container) noexcept {
    try {
        ContainerRepoItem containerItem;
        if (auto tmp = containerExist(container); tmp.has_value()) {
            containerItem = tmp.value();
        } else {
            loggerInstance()->info("container", container, "not found");
            return;
        }
        if (containerItem.status.contStatus == ContainerStatus::RUNNING) {
            loggerInstance()->info("container is running, please stop it first");
            return;
        }
        // remove files
        std::filesystem::remove_all(CONTAINER_DIR_PATH() + containerItem.containerID);
        // update repo
        {
            ContainerRepo repo;
            repo.open(CONTAINER_REPO_DB_PATH());
            repo.remove(containerItem.containerID);
            repo.close();
        }
        {
            ImageRepo repo;
            repo.open(IMAGE_REPO_DB_PATH());
            repo.updateUsedContNum(containerItem.imageID, -1);
            repo.close();
        }
    } catch (const std::exception &e) {
        loggerInstance()->error("remove container", container, "failed:", e.what());
    } catch (...) {
        loggerInstance()->error("remove container", container, "failed");
    }
}

#endif // CONTAINER_RM_CPP
