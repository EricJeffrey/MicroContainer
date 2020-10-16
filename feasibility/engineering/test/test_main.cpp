
#if !defined(TEST_MAIN_CPP)
#define TEST_MAIN_CPP

#include "test_image_repo.h"

/*
g++ -g -Wall -lleveldb -o test_main/test.out test_main.cpp *.cpp --std=c++17
*/
int main(int argc, char const *argv[]) {
    cerr << "STARTING TEST" << endl << endl;
    try {
        test_img_repo_item();
        cerr << endl;
        test_img_repo();
        cerr << endl << "ALL TEST PASSED" << endl;
    } catch (const std::exception &e) {
        cerr << "TEST FAILED: " << e.what() << endl;
    }
    return 0;
}

#endif // TEST_MAIN_CPP
