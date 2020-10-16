#if !defined(TEST_IMAGE_REPO_H)
#define TEST_IMAGE_REPO_H

#include "test_utils.h"

#include "../image_repo.h"
#include "../image_repo_item.h"
#include "../utils.h"

#include <iostream>
#include <unordered_map>
#include <vector>

using std::vector, std::unordered_map, std::cerr, std::endl;

void test_img_repo_item() {
    cerr << "-----TESTING ImageRepoItem-----" << endl;
    vector<ImageRepoItem> imgItems = {
        ImageRepoItem("docker.io", "name1", "latest", "1111", 2000, 0),
        ImageRepoItem("daocloud.io", "name2", "hahah", "2222", 3000, 3),
        ImageRepoItem("reg.ali.io", "name3", "2.2.33", "3333", now(), 1),
    };
    imgItems[0].setUsedContNum(23);
    CHECK_AND_THROW(imgItems[0].usedContNum, 23, "ImageRepoItem.setUsedContNum()");
    CHECK_AND_THROW(imgItems[0].toDBString(), string("docker.io:name1:latest:1111:2000:23"),
                    "ImageRepoItem.toDBString()");
    cerr << "-----OUTPOUT OF ImageRepoItem.toStringList()-----" << endl;
    for (auto &&item : imgItems) {
        for (auto &&str : item.toStringList())
            cerr << str << " _ ";
        cerr << endl;
    }
}

void test_img_repo() {
    cerr << "-----TESTING ImageRepo-----" << endl;
    static const string dbPath =
        "/home/eric/coding/MicroContainer/feasibility/engineering/foobar/imagerepo";
    vector<ImageRepoItem> imgRepoItems = {
        ImageRepoItem("docker.io", "name1", "latest", "1111", 2000, 0),
        ImageRepoItem("daocloud.io", "name2", "hahah", "2222", 3000, 3),
        ImageRepoItem("reg.ali.io", "name3", "2.2.33", "3333", 1258, 1),
    };
    {
        ImageRepo repo;
        repo.open(dbPath);
        for (size_t i = 0; i < 3; i++)
            repo.addImg(imgRepoItems[i].imageID, imgRepoItems[i]);
        cerr << "-----OUTPUT OF ImageRepo >> STDOUT-----" << endl;
        cerr << repo << endl;
    }
    {
        cerr << "-----TESTING READING FROM ImageRepo-----" << endl;
        ImageRepo repo;
        repo.open(dbPath);
        CHECK_AND_THROW(repo.getItem("3333").toDBString(),
                        string("reg.ali.io:name3:2.2.33:3333:1258:1"), "ImageRepo.getItem");
        CHECK_AND_THROW(repo.contains("2222"), true, "ImageRepo.contains");
        CHECK_AND_THROW(repo.contains("name1", "latest"), true, "ImageRepo.contains");
        CHECK_AND_THROW(repo.contains("name2", "latest"), false, "ImageRepo.contains");
        repo.removeImg("2222");
        CHECK_AND_THROW(repo.contains("2222"), false, "ImageRepo.contains after .removeImg");
        repo.close();
        CHECK_SHOULD_THROW(repo.contains("222"));
    }
}

#endif // TEST_IMAGE_REPO_H
