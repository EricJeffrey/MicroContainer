
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "attach.h"
#include "cleanup.h"
#include "container_ls.h"
#include "container_rm.h"
#include "create.h"
#include "image_build.h"
#include "image_ls.h"
#include "image_rm.h"
#include "lib/CLI11.hpp"
#include "network.h"
#include "pull.h"
#include "run.h"
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

        string val1, val2;
        vector<string> valList1;
        App app{"Micro management tool for containers and images", "microc"};
        app.require_subcommand(1);
        // sub command

        // container subcommand
        auto *containerSub = app.add_subcommand("container", "Manage container");
        containerSub->require_subcommand(1);
        // image subcommand
        auto *imageSub = app.add_subcommand("image", "Manage image");
        imageSub->require_subcommand(1);

        // attach
        {
            auto *sub = app.add_subcommand("attach", "Attach to a running container");
            sub->add_option("container", val1, "name or id of the container to attach");
            sub->callback([&val1]() { attach(val1); });
        }
        // build
        {
            auto *sub = app.add_subcommand("build", "Build image from Dockerfile");
            sub->add_option("file", val1, "path of file to use")->required();
            sub->add_option("-n", val2, "name of image to create");
            sub->callback([&val1, &val2]() { buildImage(val1, val2); });
        }
        // create
        {
            auto *sub = app.add_subcommand("create", "Create a container");
            sub->add_option("image", val1, "Image to use, can be name:tag or id")->required();
            sub->add_option("name", val2, "Name of the created container")->required();
            sub->callback([&val1, &val2]() { createContainer(val1, val2); });
        }
        // images
        { app.add_subcommand("images", "List images")->callback(listImages); }
        // pull
        {
            auto *pullSub1 = app.add_subcommand("pull", "Pull image from registry");
            pullSub1->add_option("image", val1, "Image name(:tag) to pull, default tag is latest")
                ->required();
            pullSub1->callback([&val1]() { pull(val1); });
        }
        // run
        {
            auto *sub =
                app.add_subcommand("run", "Runs a command in a new container from the given image");
            sub->add_option("image", val1, "id or name of image to use")->required();
            sub->add_option("-n", val2, "name of container to create");
            auto *attachFlag = sub->add_flag("-a", "attach to the started container");
            sub->add_option("args", valList1, "command and args to run in new container");
            sub->callback([&val1, &val2, &valList1, &attachFlag]() {
                runContainer(val1, valList1, val2, attachFlag->count() > 0);
            });
        }
        // start
        {
            auto *sub = app.add_subcommand("start", "Start a container");
            sub->add_option("container", val1, "Container name or id")->required();
            sub->callback([&val1]() { startContainer(val1); });
        }
        // stop
        {
            auto *sub = app.add_subcommand("stop", "Stop a running container");
            sub->add_option("container", val1, "name or id of the container to stop")->required();
            sub->callback([&val1]() {
                stop(val1);
                cleanup(val1);
            });
        }

        // container attach
        {
            auto *sub =
                containerSub->add_subcommand("attach", "Attach to a running container, after "
                                                       "attached, you can use ctrl+p to detach");
            sub->add_option("container", val1, "name or id of the container to attach")->required();
            sub->callback([&val1]() { attach(val1); });
        }
        // container cleanup
        {
            auto *sub = containerSub->add_subcommand("cleanup", "Cleanup a stopped container");
            auto *flag = sub->add_flag("-f", "stop container if running");
            sub->add_option("container", val1, "Container id")->required();
            sub->callback([&val1, &flag]() { cleanup(val1, flag->count() > 0); });
        }
        // container create
        {
            auto *ssub = containerSub->add_subcommand("create", "Create a container");
            ssub->add_option("image", val1, "Image to use, can be name:tag or id")->required();
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
        // container rm
        {
            auto *sub = containerSub->add_subcommand("rm", "Remove a container");
            sub->add_option("container", val1, "name or id of the container to remove")->required();
            sub->callback([&val1]() { removeContainer(val1); });
        }
        // container run
        {
            auto *sub = containerSub->add_subcommand(
                "run", "Runs a command in a new container from the given image");
            sub->add_option("image", val1, "id or name of image to use")->required();
            sub->add_option("-n", val2, "name of container to create");
            auto *attachFlag = sub->add_flag("-a", "attach to the started container");
            sub->add_option("args", valList1, "command and args to run in new container");
            sub->callback([&val1, &val2, &valList1, &attachFlag]() {
                runContainer(val1, valList1, val2, attachFlag->count() > 0);
            });
        }
        // contaier start
        {
            auto *sub = containerSub->add_subcommand("start", "Start a container");
            sub->add_option("container", val1, "Container name or id")->required();
            sub->callback([&val1]() { startContainer(val1); });
        }
        // container stop
        {
            auto *sub = containerSub->add_subcommand("stop", "Stop a running container");
            sub->add_option("container", val1, "name or id of the container to stop")->required();
            sub->callback([&val1]() {
                stop(val1);
                cleanup(val1);
            });
        }
        // image build
        {
            auto *sub = imageSub->add_subcommand("build", "Build image from Dockerfile");
            sub->add_option("file", val1, "path of file to use")->required();
            sub->add_option("-n", val2, "name of image to create");
            sub->callback([&val1, &val2]() { buildImage(val1, val2); });
        }
        // image ls
        { imageSub->add_subcommand("ls", "List images")->callback(listImages); }
        // image pull
        {
            auto *imgPullSub = imageSub->add_subcommand("pull", "Pull image from registry");
            imgPullSub->add_option("image", val1, "Image name(:tag) to pull, default tag is latest")
                ->required();
            imgPullSub->callback([&val1]() { pull(val1); });
        }
        // image rm
        {
            auto sub = imageSub->add_subcommand("rm", "Remove one image from local storage");
            sub->add_option("image", val1, "name:tag (tag required) or id of the image to remove")
                ->required();
            sub->callback([&val1]() { removeImage(val1); });
        }
        CLI11_PARSE(app, argc, argv);

        return 0;
    } catch (const std::exception &e) {
        cout << "Microc start failed: " << e.what() << endl;
    } catch (...) {
        cout << "Microc start failed, unknown exception" << endl;
    }
    return EXIT_FAILURE;
}

#endif // MICRO_CONTAINER_CPP
