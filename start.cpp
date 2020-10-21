#if !defined(START_CPP)
#define START_CPP

#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"
#include "network.h"
#include "sys_error.h"

#include <sys/mount.h>

#include <fstream>
#include <string>

using std::string;

/**
 * @brief mount overlay filesystem
 * @param dst destination, usually named merged
 * @param lowerDirs e.g. dir1:dir2:dir3
 * @param upperDir|workDir upper(usally named diff) dir and work dir
 * @exception SysError runtime_error
 */
int mountOverlayFs(const string &dst, const string &lowerDirs, const string &upperDir,
                   const string &workDir) {
    if (dst.empty() || upperDir.empty() || lowerDirs.empty() || workDir.empty())
        throw runtime_error("empty dir is not allowed");

    const string sep(",");
    string optionStr =
        "lowerdir=" + lowerDirs + sep + "upperdir=" + upperDir + sep + "workdir=" + workDir;
    constexpr ulong flags = MS_RELATIME | MS_NODEV;
    if (mount("overlay", dst.c_str(), "overlay", flags, optionStr.c_str()) == -1)
        throw SysError(errno, "Call to mount failed");
    return 0;
}

std::optional<ContainerRepoItem> containerExist(const string &cont) {
    ContainerRepoItem contItem;
    ContainerRepo repo;
    repo.open(CONTAINER_REPO_DB_PATH());
    if (repo.contains(cont)) {
        return {repo.getItem(cont)};
    } else {
        bool got = false;
        repo.foreach ([&cont, &contItem, &got](int i, const string &k, const string &v) {
            ContainerRepoItem item(v);
            if (item.name == cont || item.containerID.substr(0, cont.size()) == cont) {
                contItem = item;
                got = true;
                return true;
            }
            return false;
        });
        return got ? std::optional(contItem) : std::nullopt;
    }
}

/**
 * @brief start a container
 * @param cont container id or name
 */
void startContainer(const string &cont) noexcept {
    try {
        ContainerRepoItem contItem;
        // check container existence
        if (auto tmp = containerExist(cont); tmp.has_value()) {
            contItem = tmp.value();
        } else {
            loggerInstance()->info("Can not find container", cont);
            return;
        }
        string contID = contItem.containerID;
        string contIP;
        {
            IpRepo repo;
            repo.open(IPADDR_REPO_DB_PATH());
            contIP = repo.useOne();
        }
        auto [err, veth1, veth2] = createContNet(contID, contIP, VETH_IP_PREFIX_LEN);
        if (err) {
            loggerInstance()->error("Start container failed: create container network failed");
            return;
        }
        string upperDir = CONTAINER_DIR_PATH() + contID + "/diff";
        string workDir = CONTAINER_DIR_PATH() + contID + "/work";
        string mergedDir = CONTAINER_DIR_PATH() + contID + "/merged";
        // read lowers
        string lowerDirs;
        {
            std::ifstream ifs((CONTAINER_DIR_PATH() + contID).c_str());
            lowerDirs.assign((std::istreambuf_iterator<char>(ifs)),
                             (std::istreambuf_iterator<char>()));
        }
        // mount dirs
        mountOverlayFs(mergedDir, lowerDirs, upperDir, workDir);
        // todo use conmon, make args clear
    } catch (const std::exception &e) {
        loggerInstance()->error("Start container failed:", e.what());
    }
}

#endif // START_CPP
