#if !defined(CONTAINER_REPO_CPP)
#define CONTAINER_REPO_CPP

#include "container_repo.h"
#include "config.h"

#include <iomanip>
#include <memory>

void ContainerRepo::updateStatus(const string &contId, ContainerStatus contStatus, time_t time) {
    ContainerRepoItem oldItem(this->get(contId));
    oldItem.status.update(contStatus, time);
    this->update(contId, oldItem);
}
bool ContainerRepo::isImageUsed(const string &imageId) {
    checkAndThrow();
    std::unique_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));
    for (it->SeekToFirst(); it->Valid(); it->Next())
        if (ContainerRepoItem::extractImgID(it->value().ToString()) == imageId)
            return true;
    return false;
}

std::ostream &operator<<(std::ostream &out, const ContainerRepo &repo) {
    vector<vector<string>> lines;
    lines.emplace_back(ContainerRepoItem::ATTR_TAG_LIST);
    repo.foreach ([&](int i, const string &key, const string &value) {
        lines.emplace_back(ContainerRepoItem(value).toStringList());
        return false;
    });
    lineupPrint(out, lines);
    return out;
}

std::optional<ContainerRepoItem> containerExist(const string &cont) {
    ContainerRepoItem contItem;
    ContainerRepo repo;
    repo.open(CONTAINER_REPO_DB_PATH());
    if (repo.contains(cont)) {
        return {repo.getItem(cont)};
    } else {
        bool got = false;
        repo.foreach ([&cont, &contItem, &got](int i, const string &k, const string &v) {
            ContainerRepoItem item(v);
            if (item.name == cont || item.containerID.substr(0, cont.size()) == cont) {
                contItem = item;
                return got = true;
            }
            return false;
        });
        return got ? std::optional(contItem) : std::nullopt;
    }
}

#endif // CONTAINER_REPO_CPP
