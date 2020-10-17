#if !defined(DB_ERROR_H)
#define DB_ERROR_H

#include <stdexcept>
#include <string>

using std::runtime_error, std::string;

struct DBError : public runtime_error {
    DBError(const char *msg) : runtime_error(msg) {}
    DBError(const string &msg) : runtime_error(msg) {}
};

#endif // DB_ERROR_H
