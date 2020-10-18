#if !defined(IMAGE_LS_CPP)
#define IMAGE_LS_CPP

#include "image_repo.h"
#include "lib/logger.h"

#include <iostream>

void listImages() noexcept {
    try {
        ImageRepo repo;
        repo.open(IMAGE_REPO_DB_PATH);
        std::cout << repo;
        repo.close();
    } catch (const std::exception &e) {
        loggerInstance()->error("List images failed:", e.what());
    }
}

#endif // IMAGE_LS_CPP
