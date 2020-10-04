
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "app.h"
#include "pull.h"
#include "create.h"

const string appName = "microc";
const string desc = "micro management tool for containers and images";
unordered_map<string, SubCommand> commands = {
    {"pull", SubCommand(
                 1, [](const char *argv[]) { pull(argv[2]); }, "image:tag",
                 "pull image from registry/library")},
    {"create", SubCommand(
                   2, [](const char *argv[]) { createContainer(argv[2], argv[3]); }, "imageID name",
                   "create container with name using imageID")},
};

int main(int argc, char const *argv[]) {
    try {
        Logger::init(std::cout);
        loggerInstance()->setDebug(true);
        App(appName, desc, commands).start(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Pull failed with exception: " << e.what() << '\n';
    }
    return 0;
}

#endif // MICRO_CONTAINER_CPP
