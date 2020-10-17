#if !defined(SYS_ERROR_H)
#define SYS_ERROR_H

#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

using std::string;

// error when making syscall
struct SysError : protected std::exception {
    const int err;
    const string errMsg;
    SysError(int err, const char *msg) : err(err), errMsg(msg) {}
    SysError(int err, const string &msg) : err(err), errMsg(msg) {}

    string what() { return errMsg + ":" + strerror(err); }
};

#endif // SYS_ERROR_H
