
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "attach.h"
#include "cleanup.h"
#include "container_ls.h"
#include "create.h"
#include "image_ls.h"
#include "lib/CLI11.hpp"
#include "network.h"
#include "pull.h"
#include "start.h"
#include "stop.h"

#include <filesystem>

using std::cout, std::endl;
using std::make_shared, CLI::App, CLI::App_p;
using std::filesystem::is_directory, std::filesystem::create_directories;

// Things to do when first run: creating dir, init logger, create bridge
void init() {
    createBridge();
    for (auto &&path :
         {IMAGE_DIR_PATH(), CONTAINER_DIR_PATH(), OVERLAY_DIR_PATH(), LAYER_FILE_DIR_PATH(),
          SOCK_DIR_PATH(), EXIT_DIR_PATH(), string(REPO_DB_DIR_PATH)})
        if (!is_directory(path))
            create_directories(path);
    Logger::init(cout);
    loggerInstance()->setDebug(true);
}

int main(int argc, char const *argv[]) {
    if (geteuid() != 0) {
        cout << "Please run with root account" << endl;
        return 0;
    }
    try {
        init();
        {
            App app{"Micro management tool for containers and images", "microc"};
            app.require_subcommand(1);
            // sub command
            {
                // container subcommand
                auto *containerSub = app.add_subcommand("container", "Manage container");
                containerSub->require_subcommand(1);
                // image subcommand
                auto *imageSub = app.add_subcommand("image", "Manage image");
                imageSub->require_subcommand(1);

                // attach
                {
                    auto *sub = app.add_subcommand("attach", "Attach to a running container");
                    string val1;
                    sub->add_option("container", val1, "name or id of the container to attach");
                    sub->callback([&val1]() { attach(val1); });
                }
                // create
                {
                    auto *sub = app.add_subcommand("create", "Create a container");
                    string val1, val2;
                    sub->add_option("image", val1, "Image to use, can be name:tag or id")
                        ->required();
                    sub->add_option("name", val2, "Name of the created container")->required();
                    sub->callback([&val1, &val2]() { createContainer(val1, val2); });
                }
                // images
                { app.add_subcommand("images", "List images")->callback(listImages); }
                // pull
                {
                    string value;
                    auto *pullSub1 = app.add_subcommand("pull", "Pull image from registry");
                    pullSub1
                        ->add_option("image", value,
                                     "Image name(:tag) to pull, default tag is latest")
                        ->required();
                    pullSub1->callback([&value]() { pull(value); });
                }
                // start
                {
                    string value;
                    auto *sub = app.add_subcommand("start", "Start a container");
                    sub->add_option("container", value, "Container name or id")->required();
                    sub->callback([&value]() { startContainer(value); });
                }
                // stop
                {
                    auto *sub = app.add_subcommand("stop", "Stop a running container");
                    string val1;
                    sub->add_option("container", val1, "name or id of the container to stop");
                    sub->callback([&val1]() { stop(val1); });
                }

                // container attach
                {

                    auto *sub = containerSub->add_subcommand(
                        "attach", "Attach to a running container, after "
                                  "attached, you can use ctrl+p to detach");
                    string val1;
                    sub->add_option("container", val1, "name or id of the container to attach");
                    sub->callback([&val1]() { attach(val1); });
                }
                // container cleanup
                {
                    string value;
                    auto *sub =
                        containerSub->add_subcommand("cleanup", "Cleanup a stopped container");
                    sub->add_option("container", value, "Container id")->required();
                    sub->callback([&value]() { cleanup(value); });
                }
                // container create
                {
                    string val1, val2;
                    auto *ssub = containerSub->add_subcommand("create", "Create a container");
                    ssub->add_option("image", val1, "Image to use, can be name:tag or id")
                        ->required();
                    ssub->add_option("name", val2, "Name of the created container")->required();
                    ssub->callback([&val1, &val2]() { createContainer(val1, val2); });
                }
                // container ls
                {
                    auto *lsContSub = containerSub->add_subcommand("ls", "List containers");
                    auto *lsAllFLag =
                        lsContSub->add_flag("-a", "List all containers, default only running");
                    lsContSub->callback([&lsAllFLag]() { listContainer(lsAllFLag->count() > 0); });
                }
                // contaier start
                {
                    string value;
                    auto *sub = containerSub->add_subcommand("start", "Start a container");
                    sub->add_option("container", value, "Container name or id")->required();
                    sub->callback([&value]() { startContainer(value); });
                }
                // container stop
                {
                    auto *sub = containerSub->add_subcommand("stop", "Stop a running container");
                    string val1;
                    sub->add_option("container", val1, "name or id of the container to stop");
                    sub->callback([&val1]() { stop(val1); });
                }
                // image pull
                {
                    string value;
                    auto *imgPullSub = imageSub->add_subcommand("pull", "Pull image from registry");
                    imgPullSub
                        ->add_option("image", value,
                                     "Image name(:tag) to pull, default tag is latest")
                        ->required();
                    imgPullSub->callback([&value]() { pull(value); });
                }
                // image ls
                { imageSub->add_subcommand("ls", "List images")->callback(listImages); }
            }
            CLI11_PARSE(app, argc, argv);
        }
        return 0;
    } catch (const std::exception &e) {
        cout << "Microc start failed: " << e.what() << endl;
    } catch (...) {
        cout << "Microc start failed, unknown exception" << endl;
    }
    return EXIT_FAILURE;
}

#endif // MICRO_CONTAINER_CPP
