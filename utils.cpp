#if !defined(UTILS_CPP)
#define UTILS_CPP

#include "utils.h"
#include "logger.h"

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <openssl/sha.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/types.h>

string genRandomStr(int len) {
    string res;
    res.resize(len);
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < len; ++i)
        res[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    return res;
}

int fork_exec_wait(const string &filePath, const vector<string> &args, bool noOut) {
    int ret = 0;
    pid_t child = -1;
    child = fork();
    if (child == -1)
        throw runtime_error(string("Call to fork failed: ") + strerror(errno));
    else if (child == 0) {
        // child
        if (noOut) {
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
        const size_t sz = args.size();
        char *argv[sz + 1];
        for (size_t i = 0; i < sz; i++)
            argv[i] = strdup(args[i].c_str());
        argv[sz] = nullptr;
        ret = execv(filePath.c_str(), argv);
        if (ret == -1)
            exit(1);
    } else {
        int statLoc = -1;
        ret = waitpid(child, &statLoc, 0);
        if (ret == -1)
            throw runtime_error(string("Call to waitpid failed: ") + strerror(errno));
        else if (WIFEXITED(statLoc))
            return WEXITSTATUS(statLoc);
        else
            throw IncorrectlyExitError("Child returned incorrectly");
    }
    return 0;
}

string concat(const vector<string> &argv) {
    string res;
    for (auto &&x : argv)
        res.append(x).append(" ");
    return res;
};

bool isRegFileExist(const string &filePath) {
    struct stat statBuf;
    if (stat(filePath.c_str(), &statBuf) == -1) {
        const int tmpErrno = errno;
        if (tmpErrno == ENOENT)
            return false;
        else {
            loggerInstance()->sysError(errno, "Call to stat on", filePath, "failed");
            throw runtime_error("call to stat failed");
        }
    }
    return S_ISREG(statBuf.st_mode);
}

// create file with content
void createFile(const string &filePath, const string &content) {
    int fd = open(filePath.c_str(), O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH);
    if (fd == -1) {
        loggerInstance()->sysError(errno, "Call to open ", filePath, "failed");
        throw runtime_error("call to open failed");
    }
    if (write(fd, content.c_str(), content.size()) == -1) {
        loggerInstance()->sysError(errno, "Call to write failed");
        throw runtime_error("call to write failed");
    }
    if (close(fd) == -1) {
        loggerInstance()->sysError(errno, "Call to close return -1");
    }
}

string sha256_string(const char *str, size_t len) {
    char output_buffer[65];
    std::fill(output_buffer, output_buffer + 65, 0);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, len);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_buffer + (i * 2), "%02x", hash[i]);
    }
    return string(output_buffer);
}

string nowISO() {
    time_t now;
    time(&now);
    char buf[30] = {};
    strftime(buf, sizeof(buf), "%F%T%Z", gmtime(&now));
    return buf;
}

#endif // UTILS_CPP
