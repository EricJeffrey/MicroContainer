
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "pull.h"

int main(int argc, char const *argv[]) {
    try {
        Logger::init(std::cerr);
        loggerInstance()->setDebug(true);
        pull("python", "alpine");
    } catch (const std::exception &e) {
        std::cerr << "Pull failed with exception: " << e.what() << '\n';
    }
    return 0;
}

#endif // MICRO_CONTAINER_CPP
