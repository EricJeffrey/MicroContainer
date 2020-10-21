#if !defined(CLEANUP_CPP)
#define CLEANUP_CPP

#include "cleanup.h"
#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"
#include "network.h"

#include <sys/mount.h>

void cleanup(const string &contID) noexcept {
    if (umount((CONTAINER_DIR_PATH() + contID + "/merged").c_str()) == -1)
        loggerInstance()->error("Call to umount failed:", strerror(errno));
    cleanupContNet(contID);
    // update repo db
    {
        ContainerRepo repo;
        repo.open(CONTAINER_REPO_DB_PATH());
        repo.updateStatus(contID, ContainerStatus::STOPPED, now());
    }
}

#endif // CLEANUP_CPP
