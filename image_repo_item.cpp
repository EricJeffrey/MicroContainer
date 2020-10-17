#if !defined(IMAGE_REPO_ITEM_CPP)
#define IMAGE_REPO_ITEM_CPP

#include "image_repo_item.h"
#include "utils.h"

using std::runtime_error;

string ImageRepoItem::toDBString() const {
    return regAddr + SEP + name + SEP + tag + SEP + imageID + SEP + std::to_string(created) + SEP +
           std::to_string(usedContNum);
}
// create item from database string, throw runtime_error
ImageRepoItem::ImageRepoItem(const string &itemStr) {
    auto check = [](size_t pos) {
        if (pos == string::npos)
            throw runtime_error("cannot create ImageRepoItem: invalid db string value");
    };
    size_t laPos = 0, curPos;
    auto work = [&]() {
        check(curPos = itemStr.find(SEP, laPos));
        string res = itemStr.substr(laPos, curPos - laPos);
        laPos = curPos + 1;
        return res;
    };
    regAddr = work();
    name = work();
    tag = work();
    imageID = work();
    created = std::stol(work());
    usedContNum = std::stoi(itemStr.substr(laPos));
}

vector<string> ImageRepoItem::toStringList() const {
    return {regAddr, name, tag, imageID, timet2Str(created)};
}

vector<string> ImageRepoItem::ATTR_TAG_LIST = {"REPOSITORY", "NAME", "TAG", "IMAGE ID", "CREATED"};

#endif // IMAGE_REPO_ITEM_CPP
