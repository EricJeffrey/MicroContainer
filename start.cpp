#if !defined(START_CPP)
#define START_CPP

#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"
#include "network.h"
#include "sys_error.h"

#include <fcntl.h>
#include <sys/mount.h>

#include <filesystem>
#include <fstream>
#include <string>

using std::string;

/**
 * @brief mount overlay filesystem
 * @param dst destination, usually named merged
 * @param lowers e.g. layer1:layer2:layer3
 * @param upperDir|workDir upper(usally named diff) dir and work dir
 * @exception SysError runtime_error
 */
int mountOverlayFs(const string &dst, const string &lowers, const string &upperDir,
                   const string &workDir) {
    if (dst.empty() || upperDir.empty() || lowers.empty() || workDir.empty())
        throw runtime_error("empty dir is not allowed");

    string lowerDirs;
    {
        auto layers = split(lowers, ":");
        for (size_t i = 0; i < layers.size(); i++) {
            if (i != 0)
                lowerDirs += ":";
            lowerDirs += OVERLAY_DIR_PATH() + layers[i];
        }
    }

    const string sep(",");
    string optionStr =
        "lowerdir=" + lowerDirs + sep + "upperdir=" + upperDir + sep + "workdir=" + workDir;
    constexpr ulong flags = MS_RELATIME | MS_NODEV;
    if (mount("overlay", dst.c_str(), "overlay", flags, optionStr.c_str()) == -1)
        throw SysError(errno, "Call to mount failed");
    return 0;
}

void prepareFiles(const string &contID) {
    const auto contDir = CONTAINER_DIR_PATH() + contID + "/";
    for (auto &&filename :
         {CONTAINER_PID_FILENAME, CONMON_PID_FILENAME, RUNTIME_LOG_FILENAME, CONMON_LOG_FILENAME}) {
        ofstream((contDir + USERDATA_DIR_NAME + filename).c_str());
    }
}

/**
 * @brief fork and execv(conmon), auto append nullptr to args
 * @return true if execv success, failed otherwise
 * @exception SysError
 */
bool execConmon(vector<string> &args) {
    int syncpfd[2];
    if (pipe(syncpfd) == -1)
        throw SysError(errno, "call to pipe failed");
    pid_t child = fork();
    if (child < 0) {
        throw SysError(errno, "call to fork failed");
    } else if (child == 0) {
        // child
        close(syncpfd[0]);
        // conmon will send us {"pid": value(-1 error), "message": "xxx"} using this pipe
        if (setenv("_OCI_SYNCPIPE", std::to_string(syncpfd[1]).c_str(), 1) == -1)
            throw SysError(errno, "call to set env failed");
        const size_t sz = args.size();
        char *argv[sz + 1];
        for (size_t i = 0; i < sz; i++)
            argv[i] = args[i].data();
        argv[sz] = nullptr;
        execv(CONMON_PATH, argv);
        std::cerr << "Call to exec-conmon failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    // parent
    close(syncpfd[1]);
    bool res = false;
    char buf[BUFSIZ] = {};
    int num = read(syncpfd[0], buf, BUFSIZ);
    if (num < 0)
        throw SysError(errno, "read on sync pipe failed");
    else if (num == 0)
        loggerInstance()->error("sync pipe read return 0");
    else {
        nlohmann::json j = nlohmann::json::parse(buf);
        // conmon will write "data" to it
        int containerPid = j["data"].get<int>();
        if (containerPid == -1) {
            string msg = "failed to read container pid from conmon";
            if (j.contains("message"))
                msg.append(": ").append(j["message"].get<string>());
            loggerInstance()->error(msg);
        } else {
            res = true;
        }
    }
    close(syncpfd[0]);
    return res;
}

/**
 * @brief start a container
 * @param cont container id or name
 */
void startContainer(const string &cont) noexcept {
    ContainerRepoItem contItem;
    string mergedDir;
    auto cleanup = [&contItem, &mergedDir]() {
        if (!contItem.containerID.empty())
            cleanupContNet(contItem.containerID);
        if (!mergedDir.empty())
            umount(mergedDir.c_str());
    };
    try {
        // check container existence and not running
        {
            if (auto tmp = containerExist(cont); tmp.has_value()) {
                contItem = tmp.value();
            } else {
                loggerInstance()->info("Can not find container", cont);
                return;
            }
            if (contItem.status.contStatus == ContainerStatus::RUNNING) {
                loggerInstance()->info("Container", cont, "is running");
                return;
            }
            if (contItem.status.contStatus == ContainerStatus::INVALID) {
                loggerInstance()->error("Start container failed, container status unknown");
                return;
            }
        }
        string contID = contItem.containerID;
        // prepare log/pid files
        prepareFiles(contID);
        // FIXME need to store ip address in repo
        // get an ip address for use
        string contIP;
        {
            IpRepo repo;
            repo.open(IPADDR_REPO_DB_PATH());
            contIP = repo.useOne();
        }
        // create netns
        {
            auto [err, veth1, veth2] = createContNet(contID, contIP, VETH_IP_PREFIX_LEN);
            if (err) {
                loggerInstance()->error("Start container failed: create container network failed");
                return;
            }
        }
        const string contDir = CONTAINER_DIR_PATH() + contID + "/";
        const string contUsrDataDir = contDir + "userdata/";
        // mount overlay filesystem
        {
            mergedDir = contDir + "merged";
            string upperDir = contDir + "diff";
            string workDir = contDir + "work";
            // read lowers
            std::ifstream ifs((contDir + "lower").c_str());
            string lowerDirs((std::istreambuf_iterator<char>(ifs)),
                             (std::istreambuf_iterator<char>()));
            mountOverlayFs(mergedDir, lowerDirs, upperDir, workDir);
        }
        vector<string> args = {
            CONMON_NAME, "-s", "-t", "--api-version", "1", "-c", contID, "-u", contID, "-n",
            contItem.name, "-b", contDir, "-p", (contUsrDataDir + CONTAINER_PID_FILENAME),
            "--socket-dir-path", SOCK_DIR_PATH(), "-l", (contUsrDataDir + CONMON_LOG_FILENAME),
            "--log-level", "error", "-r", CONTAINER_RT_PATH, "--runtime-arg", "--log-format=json",
            "--runtime-arg", "--log", "--runtime-arg", (contUsrDataDir + RUNTIME_LOG_FILENAME),
            "--conmon-pidfile", (contUsrDataDir + CONMON_PID_FILENAME), "--exit-dir",
            EXIT_DIR_PATH(), "--exit-delay", "1", "--exit-command",
            // todo change it to a fixed path
            "/home/eric/coding/MicroContainer/microc", "--exit-command-arg", "container",
            "--exit-command-arg", "cleanup", "--exit-command-arg", "-f", "--exit-command-arg",
            contID};
        if (execConmon(args)) {
            if (fork_exec_wait(CONTAINER_RT_PATH, {CONTAINER_RT_NAME, "start", contID}) != 0)
                throw runtime_error("crun start failed");
            { // update repo
                ContainerRepo repo;
                repo.open(CONTAINER_REPO_DB_PATH());
                repo.updateStatus(contID, ContainerStatus::RUNNING, now());
                repo.close();
            }
            loggerInstance()->info("started", contID);
        } else {
            throw runtime_error("exec conmon failed");
        }
    } catch (const std::exception &e) {
        loggerInstance()->error("Start container failed:", e.what());
        cleanup();
    }
}

#endif // START_CPP
