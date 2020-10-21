#if !defined(IMAGE_REPO_ITEM_H)
#define IMAGE_REPO_ITEM_H

#include "repo_item.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using std::vector, std::string;

/**
 * item stored in local image repository
 */
struct ImageRepoItem : public RepoItem {
    static vector<string> ATTR_TAG_LIST;
    static constexpr char SEP = ':';

    // address of registry, e.g. docker.io/library/
    string regAddr;
    string name;
    string tag;
    string imageID;
    time_t created;
    int usedContNum; /* container number that is using this image */

    ImageRepoItem(string regAddr, string name, string tag, string id, time_t created, int useCnt)
        : regAddr(regAddr), name(name), tag(tag), imageID(id), created(created),
          usedContNum(useCnt) {}

    ImageRepoItem(const string &dbStr);
    ~ImageRepoItem() {}

    void setUsedContNum(int newNum) { usedContNum = newNum; }
    string toDBString() const override;
    vector<string> toStringList() const;
};

#endif // IMAGE_REPO_ITEM_H
