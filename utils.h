#if !defined(UTILS_H)
#define UTILS_H

#include <string>
#include <vector>
#include <stdexcept>
#include <exception>
using std::runtime_error;

using std::string;
using std::vector;

typedef std::pair<int, bool> PairIntBool;
typedef std::tuple<string, int, bool> TupStrIntBool;

class IncorrectlyExitError : public runtime_error {
public:
    IncorrectlyExitError(const char *what) : runtime_error(what) {}
    ~IncorrectlyExitError() {}
};

// generate random str with alpha & digit
string genRandomStr(int len);

// fork & exec in child, wait child and get return value
// args should contain filename, close stdout/stderr if noOut is true [default]
// throw on error and when child not returned correctly (not WIFEXITED)
int fork_exec_wait(const string &filePath, const vector<string> &args, bool noOut = false);

// concatenate a vector<string> with space
string concat(const vector<string> &argv);

// check if regular file exist
bool isRegFileExist(const string &filePath);

void createFile(const string &filePath, const string &content);

// get sha256 of [str]
string sha256_string(const char *str, size_t len);

// get current iso time
string nowISO();

#endif // UTILS_H
