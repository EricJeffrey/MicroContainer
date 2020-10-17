
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "app.h"
#include "create.h"
#include "pull.h"

#include <filesystem>

using std::filesystem::is_directory, std::filesystem::create_directories, std::cerr, std::endl;

const string appName = "microc";
const string description = "micro management tool for containers and images";
unordered_map<string, SubCommand> commands = {
    {"pull", SubCommand(
                 1, [](const char *argv[]) { pull(argv[2]); }, "image:tag",
                 "pull image from registry/library")},
    {"create", SubCommand(
                   2, [](const char *argv[]) { createContainer(argv[2], argv[3]); }, "imageID name",
                   "create container with name using imageID")},
};

/**
 * @brief Things to do when first run: creating dir, init logger, create bridge
 * @return true if no exception, false otherwise
 */
bool init() {
    // todo create bridge
    try {
        for (auto &&path : {IMAGE_DIR_PATH, CONTAINER_DIR_PATH, OVERLAY_DIR_PATH,
                            LAYER_FILE_DIR_PATH, REPO_DB_DIR_PATH})
            if (!is_directory(path))
                create_directories(path);
        Logger::init(std::cout);
        loggerInstance()->setDebug(true);
        return true;
    } catch (std::filesystem::filesystem_error &e) {
        cerr << "Create repository dir failed" << endl;
    } catch (...) {
        cerr << "failed to init microc" << endl;
    }
    return false;
}

int main(int argc, char const *argv[]) {
    if (init()) {
        try {
            App(appName, description, commands).start(argc, argv);
            return 0;
        } catch (const std::exception &e) {
            std::cerr << "Command failed with exception: " << e.what() << '\n';
        }
    }
    return EXIT_FAILURE;
}

#endif // MICRO_CONTAINER_CPP
