#if !defined(UTILS_H)
#define UTILS_H

#include "sys_error.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using std::string, std::vector, std::runtime_error;

// error indicate that child(by fork) exited incorrectly
class IncorrectlyExitError : public runtime_error {
public:
    IncorrectlyExitError(const char *what) : runtime_error(what) {}
    ~IncorrectlyExitError() {}
};

inline vector<string> split(const string &str, const string &delimiter) {
    vector<string> res;
    size_t laPos = 0, curPos, sizeDeli = delimiter.size();
    if (sizeDeli == 0)
        return {};
    curPos = str.find_first_of(delimiter);
    while (curPos != string::npos) {
        res.emplace_back(str.substr(laPos, curPos - laPos));
        laPos = curPos + sizeDeli;
        curPos = str.find_first_of(delimiter, laPos);
    }
    if (laPos < sizeDeli)
        res.emplace_back(str.substr(laPos));
    return res;
}

/**
 * @brief get current time
 */
inline time_t now() noexcept {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

/**
 * @brief convert time_t to string format, using [std::put_time] with format(default "%F %R")
 */
inline string timet2Str(time_t time, const string &format = "%F %R") {
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), format.c_str());
    return ss.str();
}
/**
 * check if path is regular file
 * @exception SysError
 */
inline bool isRegFile(const string &path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) == -1) {
        const int tmpErrno = errno;
        if (tmpErrno == ENOENT)
            return false;
        else
            throw SysError(tmpErrno, "call to stat failed");
    }
    return S_ISREG(statBuf.st_mode);
}

/**
 * @brief fork & exec an program with args
 * @param noStdIO close stderr/stdout/stdin if true
 * @return child exit value
 */
inline int fork_exec_wait(const string &filePath, const vector<string> &args, bool noStdIO) {
    pid_t child = fork();
    if (child == -1)
        throw SysError(errno, "Call to fork failed");
    else if (child == 0) { // child
        if (noStdIO)
            close(STDOUT_FILENO), close(STDERR_FILENO), close(STDIN_FILENO);
        const size_t sz = args.size();
        char *argv[sz + 1];
        for (size_t i = 0; i < sz; i++)
            argv[i] = strdup(args[i].c_str());
        argv[sz] = nullptr;
        if (execv(filePath.c_str(), argv) == -1)
            exit(1);
    } else { // parent
        int statLoc = -1;
        if (waitpid(child, &statLoc, 0) == -1)
            throw SysError(errno, "Call to waitpid failed");
        else if (WIFEXITED(statLoc))
            return WEXITSTATUS(statLoc);
        else
            throw IncorrectlyExitError("Child returned incorrectly");
    }
}

/**
 * @brief create netns of name, stored in /var/run/netns/name.
 * using "ip netns add name" (fork & exec) to create.
 * @return true if successfully create netns, otherwise false
 * @exception SysError, IncorrectlyExitError
 */
inline bool createNetns(const string &name) {
    return fork_exec_wait("/usr/sbin/ip", {"ip", "netns", "add", name}, true) == 0;
}

#endif // UTILS_H
