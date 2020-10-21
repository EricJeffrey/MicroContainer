#if !defined(TEST_CONTAINER_REPO_H)
#define TEST_CONTAINER_REPO_H

#include "../config.h"
#include "../container_repo.h"

#include "test_utils.h"

void test_container_repo() {
    cerr << "-----TESTING CONTAINER REPO-----" << endl;
    using Status = ContainerRepoItem::Status;
    vector<ContainerRepoItem> items = {
        ContainerRepoItem("cont111", "1111", "conname1", "sh", 23333, Status(RUNNING, 1111)),
        ContainerRepoItem("cont222", "2222", "conname2", "py", now(),
                          Status(STOPPED, now() - 10000000)),
        ContainerRepoItem("cont333", "1111", "conname3", "cat", 11111, Status(CREATED, 11111)),
    };
    CHECK_AND_THROW(items[0].toDBString(), string("cont111|1111|conname1|sh|23333|1_1111"),
                    "ContainerRepoItem.toDBString");
    CHECK_AND_THROW(ContainerRepoItem::extractImgID("cont111|1111|conname1|sh|23333|1_1111"),
                    string("1111"), "ContainerRepoItem.extractImgID");
    {
        ContainerRepo repo;
        repo.open(CONTAINER_REPO_DB_PATH());
        for (auto &&item : items)
            repo.add(item.containerID, item);
        CHECK_AND_THROW(repo.contains("cont111"), true, "ContainerRepo.contains");
        CHECK_AND_THROW(repo.contains("0000"), false, "ContainerRepo.contains");
        CHECK_AND_THROW(repo.isImageUsed("1111"), true, "ContainerRepo.isImageUsed");
        CHECK_AND_THROW(repo.isImageUsed("3333"), false, "ContainerRepo.isImageUsed");
        cerr << "-----OUTPUT OF ContainerRepo >> stdout----" << endl;
        cerr << repo << endl;
        repo.close();
        CHECK_SHOULD_THROW(repo.add("4444", items[0]));
    }
}

#endif // TEST_CONTAINER_REPO_H
