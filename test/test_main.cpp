
#if !defined(TEST_MAIN_CPP)
#define TEST_MAIN_CPP

#include "test_image_repo.h"
#include "test_container_repo.h"
#include "test_cont_net.h"

int main(int argc, char const *argv[]) {
    cerr << "STARTING TEST" << endl << endl;
    try {
        test_container_net();
        cerr << endl << "ALL TEST PASSED" << endl;
    } catch (const std::exception &e) {
        cerr << "TEST FAILED: " << e.what() << endl;
    }
    return 0;
}

#endif // TEST_MAIN_CPP
