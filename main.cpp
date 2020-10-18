
#if !defined(MICRO_CONTAINER_CPP)
#define MICRO_CONTAINER_CPP

#include "container_ls.h"
#include "create.h"
#include "image_ls.h"
#include "lib/CLI11.hpp"
#include "pull.h"

#include <filesystem>

using std::cout, std::endl;
using std::filesystem::is_directory, std::filesystem::create_directories;

// Things to do when first run: creating dir, init logger, create bridge
void init() {
    for (auto &&path : {IMAGE_DIR_PATH, CONTAINER_DIR_PATH, OVERLAY_DIR_PATH, LAYER_FILE_DIR_PATH,
                        REPO_DB_DIR_PATH})
        if (!is_directory(path))
            create_directories(path);
    Logger::init(cout);
    loggerInstance()->setDebug(true);
    // todo create bridge here
}

int main(int argc, char const *argv[]) {
    using std::make_shared, CLI::App, CLI::App_p;
    try {
        init();
        {
            // Option with name, description and value
            struct OptAggr {
                string name, desc, val;
                OptAggr(string n, string d) : name(n), desc(d) {}
            };
            // Sub command with name and desc
            struct SubCmdAggr {
                string name, desc;
                SubCmdAggr(string n, string d) : name(n), desc(d) {}
            };
            App app{"Micro management tool for containers and images", "microc"};
            app.require_subcommand(1);
            // container sub command
            {
                auto *containerSub = app.add_subcommand("container", "Manage container");
                containerSub->require_subcommand(1);
                // create & container create
                {
                    SubCmdAggr subcCreat("create", "Create container");
                    auto *createSub1 = app.add_subcommand(subcCreat.name, subcCreat.desc);
                    OptAggr optName("name", "Name of the created container");
                    OptAggr optImg("image", "Image to use, can be name:tag or id");
                    auto createCb = [&optName, &optImg]() {
                        createContainer(optImg.val, optName.val);
                    };
                    createSub1->add_option(optImg.name, optImg.val, optImg.desc)->required();
                    createSub1->add_option(optName.name, optName.val, optName.desc)->required();
                    createSub1->callback(createCb);

                    auto *contCreateSub =
                        containerSub->add_subcommand(subcCreat.name, subcCreat.desc);
                    contCreateSub->add_option(optImg.name, optImg.val, optImg.desc)->required();
                    contCreateSub->add_option(optName.name, optName.val, optName.desc)->required();
                    contCreateSub->callback(createCb);
                }
                { // container ls
                    auto *lsContSub = containerSub->add_subcommand("ls", "List containers");
                    auto *lsAllFLag =
                        lsContSub->add_flag("-a", "List all containers, default only running");
                    lsContSub->callback([&lsAllFLag]() { listContainer(lsAllFLag->count() > 0); });
                }
            }
            // image sub command
            {
                auto *imageSub = app.add_subcommand("image", "Manage image");
                imageSub->require_subcommand(1);
                // pull & image pull
                {
                    SubCmdAggr subcPull("pull", "Pull image from registry");
                    OptAggr optName("image", "Image name(:tag) to pull, default tag is latest");
                    auto pullCb = [&optName]() { pull(optName.val); };

                    auto *pullSub1 = app.add_subcommand(subcPull.name, subcPull.desc);
                    pullSub1->add_option(optName.name, optName.val, optName.desc)->required();
                    pullSub1->callback(pullCb);

                    auto *imgPullSub = imageSub->add_subcommand(subcPull.name, subcPull.desc);
                    imgPullSub->add_option(optName.name, optName.val, optName.desc)->required();
                    imgPullSub->callback(pullCb);
                }
                // image ls
                {
                    imageSub->add_subcommand("ls", "List images")->callback([]() { listImages(); });
                }
            }
            CLI11_PARSE(app, argc, argv);
        }
        return 0;
    } catch (const std::exception &e) {
        cout << "Microc start failed: " << e.what() << endl;
    } catch (...) {
        cout << "Microc start failed, unknown exception throwed" << endl;
    }
    return EXIT_FAILURE;
}

#endif // MICRO_CONTAINER_CPP
