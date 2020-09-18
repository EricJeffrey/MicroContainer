#if !defined(UTILS_CPP)
#define UTILS_CPP

#include "utils.h"

#include <ctime>
#include <cstdlib>
#include <cstring>

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

#endif // UTILS_CPP
