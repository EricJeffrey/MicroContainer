#if !defined(SYS_ERROR_H)
#define SYS_ERROR_H

#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

using std::string;

// error when making syscall
struct SysError : public std::exception {
    const int err;
    const string errMsg;
    SysError(int err, const char *msg) : err(err), errMsg(string(msg) + ": " + strerror(err)) {}
    SysError(int err, const string &msg) : err(err), errMsg(msg + ": " + strerror(err)) {}

    const char *what() const noexcept override { return errMsg.c_str(); }
};

#endif // SYS_ERROR_H
