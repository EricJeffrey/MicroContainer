#if !defined(CONTAINER_REPO_ITEM_CPP)
#define CONTAINER_REPO_ITEM_CPP

#include "container_repo_item.h"

#include <stdexcept>

using std::runtime_error;

string ContainerRepoItem::Status::toString() const {
    switch (contStatus) {
        case CREATED:
            return "CREATED";
        case RUNNING:
            return "UP   SINCE " + timet2Str(time);
        case STOPPED:
            return "STOPPED AT " + timet2Str(time);
        default:
            return "INVALID";
    }
}

string ContainerRepoItem::Status::toDBString() const {
    return std::to_string(contStatus) + Status::SEP + std::to_string(time);
}

/**
 * @brief construct from database string
 * @exception runtime_error
 */
ContainerRepoItem::Status::Status(const string &dbStr) {
    do {
        if (dbStr.size() <= 1)
            break;
        size_t contStaPos = dbStr.find(Status::SEP);
        if (contStaPos == dbStr.npos || contStaPos == 0 || contStaPos + 1 == dbStr.size())
            break;
        int contStatusInt = std::stoi(dbStr.substr(0, contStaPos));
        this->contStatus =
            (contStatusInt == 0
                 ? CREATED
                 : (contStatusInt == 1 ? RUNNING : (contStatusInt == 2 ? STOPPED : INVALID)));
        this->time = std::stol(dbStr.substr(contStaPos + 1));
        return;
    } while (true);
    throw runtime_error("failed to construct container status, invalid database string");
}

ContainerRepoItem::ContainerRepoItem(const string &itemStr) {
    auto check = [&](size_t pos) {
        if (pos == itemStr.npos)
            throw runtime_error("cannot create ContainerRepoItem: invalid db string value");
    };
    size_t laPos = 0, curPos;
    auto work = [&]() {
        check(curPos = itemStr.find(SEP, laPos));
        string res = itemStr.substr(laPos, curPos - laPos);
        laPos = curPos + 1;
        return res;
    };
    containerID = work();
    imageID = work();
    name = work();
    command = work();
    created = std::stol(work());
    status = Status(itemStr.substr(laPos));
}

vector<string> ContainerRepoItem::toStringList() {
    return {containerID.substr(0, 12), imageID.substr(0, 12), name, command,
            timet2Str(created),        status.toString()};
}

string ContainerRepoItem::toDBString() const {
    return containerID + SEP + imageID + SEP + name + SEP + command + SEP +
           std::to_string(created) + SEP + status.toDBString();
}

/**
 * @brief extract image id from a item db string
 * @exception runtime_error when not found
 */
string ContainerRepoItem::extractImgID(const string &str) {
    size_t pos = str.find(SEP);
    if (pos != string::npos) {
        pos += 1;
        size_t newPos = str.find(SEP, pos);
        if (newPos != string::npos)
            return str.substr(pos, newPos - pos);
    }
    throw runtime_error("extract image id failed, invalid db string");
}

vector<string> ContainerRepoItem::ATTR_TAG_LIST = {
    "CONTAINER ID", "IMAGE", "NAME", "COMMAND", "CREATED", "STATUS",
};

#endif // CONTAINER_REPO_ITEM_CPP
