#if !defined(IMAGE_RM_CPP)
#define IMAGE_RM_CPP

#include "config.h"
#include "container_repo.h"
#include "image_repo.h"
#include "lib/logger.h"

#include <filesystem>

void removeImage(const string &image) noexcept {
    try {
        string imageId;
        if (auto tmp = imageExist(image); tmp.has_value()) {
            imageId = tmp.value().imageID;
        } else {
            loggerInstance()->info("image", image,
                                   (image.find(':') != string::npos)
                                       ? "not found, note tag is required with name"
                                       : "not found");
            return;
        }
        {
            ContainerRepo repo;
            repo.open(CONTAINER_REPO_DB_PATH());
            if (repo.isImageUsed(imageId)) {
                loggerInstance()->info("image", image, "is used by container");
                return;
            }
        }
        std::filesystem::remove_all(IMAGE_DIR_PATH() + imageId);
        {
            ImageRepo repo;
            repo.open(IMAGE_REPO_DB_PATH());
            repo.removeImg(imageId);
        }
        // todo should remove unused layers either
    } catch (const std::exception &e) {
        loggerInstance()->error("failed to remove image:", e.what());
    } catch (...) {
        loggerInstance()->error("failed to remove image:");
    }
};

#endif // IMAGE_RM_CPP
