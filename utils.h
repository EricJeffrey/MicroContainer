#if !defined(UTILS_H)
#define UTILS_H

#include <chrono>
#include <exception>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "sys_error.h"

#include <cstdlib>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using std::runtime_error;

using std::string;
using std::vector;

typedef std::pair<int, bool> PairIntBool;
typedef std::tuple<string, int, bool> TupStrIntBool;

// error indicate that child(by fork) exited incorrectly
class IncorrectlyExitError : public runtime_error {
public:
    IncorrectlyExitError(const char *what) : runtime_error(what) {}
    ~IncorrectlyExitError() {}
};

vector<string> split(const string &str, const string &delimiter);

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
 * @brief fork & exec an program with args,
 * wait for the child and return its exit value
 * @param noStdIO close stderr/stdout/stdin if true
 * @return child exit value
 * @exception SysError IncorrectlyExitError
 */
int fork_exec_wait(const string &filePath, const vector<string> &args, bool noStdIO = false);

/**
 * @brief create netns of name, stored in /var/run/netns/name.
 * using "ip netns add name" (fork & exec) to create.
 * @return true if successfully create netns, otherwise false
 * @exception system_error, IncorrectlyExitError
 */
inline bool createNetns(const string &name) {
    return fork_exec_wait("/usr/sbin/ip", {"ip", "netns", "add", name}, true) == 0;
}

// generate random str with alpha & digit
string genRandomStr(int len);

/**
 * @brief concatenate an for_range iterable lislt with space
 */
string concat(const vector<string> &argv);

void createFile(const string &filePath, const string &content);

// get sha256 of [str]
string sha256_string(const char *str, size_t len);

// get current iso time
string nowISO();

#endif // UTILS_H
