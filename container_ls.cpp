#if !defined(CONTAINER_LS_CPP)
#define CONTAINER_LS_CPP

#include "container_ls.h"
#include "config.h"
#include "container_repo.h"
#include "db_error.h"
#include "lib/logger.h"

#include <iostream>

void listContainer(bool all) noexcept {
    try {
        ContainerRepo repo;
        repo.open(CONTAINER_REPO_DB_PATH());
        if (all) {
            std::cout << repo;
        } else {
            vector<vector<string>> lines;
            lines.emplace_back(ContainerRepoItem::ATTR_TAG_LIST);
            repo.foreach ([&lines](int i, const string &key, const string &value) {
                ContainerRepoItem &&item = ContainerRepoItem(value);
                if (item.status.contStatus == RUNNING)
                    lines.emplace_back(item.toStringList());
                return false;
            });
            lineupPrint(std::cout, lines);
        }
        repo.close();
    } catch (const std::exception &e) {
        loggerInstance()->error("List container failed:", e.what());
    }
}

#endif // CONTAINER_LS_CPP
