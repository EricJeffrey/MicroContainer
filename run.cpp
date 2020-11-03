#if !defined(RUN_CPP)
#define RUN_CPP

#include "attach.h"
#include "create.h"
#include "image_repo.h"
#include "lib/logger.h"
#include "start.h"
#include "utils.h"

void runContainer(const string &image, const vector<string> &args, const string &name,
                  bool doAttach) noexcept {
    try {
        string imageId;
        if (auto tmp = imageExist(image); tmp.has_value()) {
            imageId = tmp.value().imageID;
        } else {
            loggerInstance()->info("Image", image, "not found");
            return;
        }
        string containerName = name;
        if (containerName.empty())
            containerName = genRandomStr(6) + "_" + genRandomStr(6);
        string containerId = sha256_string(containerName.c_str(), containerName.size());
        createContainer(imageId, containerName, args);
        startContainer(containerId);
        if (doAttach)
            attach(containerId);
    } catch (const std::exception &e) {
        loggerInstance()->error("run container failed:", e.what());
    } catch (...) {
        loggerInstance()->error("run container failed:");
    }
}

#endif // RUN_CPP
