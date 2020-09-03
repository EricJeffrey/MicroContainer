
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "pull.h"

// todo get image config

void runCommand(int argc, const char *argv[]) {
    if (argc == 3 && string(argv[1]) == "pull") {
        pull(argv[2]);
    } else {
        std::cerr << "supported command: " << endl;
        std::cerr << argv[0] << " pull name:tag" << endl;
    }
}

int main(int argc, char const *argv[]) {
    try {
        Logger::init(std::cerr);
        loggerInstance()->setDebug(true);
        runCommand(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Pull failed with exception: " << e.what() << '\n';
    }
    return 0;
}

#endif // MICRO_CONTAINER_CPP
