#if !defined(STOP_CPP)
#define STOP_CPP

#include "stop.h"
#include "cleanup.h"
#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"

#include <filesystem>
#include <istream>
#include <iterator>

// todo see whether conmon will call cleanup or not
bool stop(const string &container) noexcept {
    try {
        string containerId;
        {
            ContainerRepoItem targetItem;
            if (auto tmp = containerExist(container); tmp.has_value())
                targetItem = tmp.value();
            else {
                loggerInstance()->info("container", container, "not found");
                return false;
            }
            if (targetItem.status.contStatus != ContainerStatus::RUNNING) {
                loggerInstance()->info("container", container, "is not running");
                return false;
            }
            if (targetItem.containerID.empty()) {
                loggerInstance()->error("failed to get container status: empty id");
                return false;
            }
            containerId = targetItem.containerID;
        }
        pid_t containerPid;
        string pidStr;
        {
            std::ifstream pidIfs(CONTAINER_DIR_PATH() + containerId + "/" + USERDATA_DIR_NAME +
                                 "/" + CONTAINER_PID_FILENAME);
            pidStr =
                string((std::istreambuf_iterator<char>(pidIfs)), std::istreambuf_iterator<char>());
            containerPid = std::stoi(pidStr);
            if (containerPid <= 0) {
                loggerInstance()->error("invalid container pid read from database");
                return false;
            }
        }
        // update status to stop
        {
            ContainerRepo repo;
            repo.open(CONTAINER_REPO_DB_PATH());
            repo.updateStatus(containerId, ContainerStatus::STOPPED, now());
            repo.close();
        }
        // timer to see whether container process exited
        {
            if (kill(containerPid, SIGTERM) == -1) {
                loggerInstance()->sysError(errno, "stop(SIGTERM) container process failed");
                return false;
            }
            int sec = 5;
            const string procPath = string("/proc/") + pidStr;
            for (; sec > 0; sec--)
                if (std::filesystem::is_directory(procPath))
                    sleep(1);
                else
                    break;
            if (sec == 0) {
                if (kill(containerPid, SIGKILL) == -1) {
                    loggerInstance()->sysError(errno, "stop(SIGKILL) container process failed");
                    return false;
                }
            }
        }
        cleanup(containerId);
        loggerInstance()->info(containerId);
        return true;
    } catch (const std::exception &e) {
        loggerInstance()->error("stop container", container, "failed:", e.what());
    } catch (...) {
        loggerInstance()->error("stop container", container, "failed:");
    }
    return false;
}

#endif // STOP_CPP
