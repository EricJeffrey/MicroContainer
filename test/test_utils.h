#if !defined(TEST_UTILS_H)
#define TEST_UTILS_H

#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>

using std::string, std::runtime_error, std::endl, std::cerr;

template <typename T> void CHECK_AND_THROW(const T &lhs, const T &rhs, const string &msg) {
    cerr << "TESTING EXPRESSION EQUAL: ";
    if (lhs != rhs) {
        cerr << "FAILED!\n\tlhs rhs: " << lhs << "\t" << rhs << endl;
        throw runtime_error(msg + " failed");
    }
    cerr << "PASSED" << endl;
}

#define CHECK_SHOULD_THROW(A)                                                                      \
    try {                                                                                          \
        cerr << "TESTING SHOULD THROW: ";                                                          \
        A;                                                                                         \
        cerr << "FAILED! SHOULD BUT NOT THROW";                                                    \
    } catch (const std::exception &e) {                                                            \
        cerr << "PASSED! THROW EXCEPTION: " << typeid(e).name() << ": " << e.what() << endl;       \
    }

#endif // TEST_UTILS_H
