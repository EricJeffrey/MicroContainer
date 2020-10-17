#if !defined(CONTAINER_REPO_CPP)
#define CONTAINER_REPO_CPP

#include "container_repo.h"

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
    vector<int> widths;
    for (auto &&v : ContainerRepoItem::ATTR_TAG_LIST)
        widths.push_back(v.size());
    vector<vector<string>> lines;
    repo.foreach ([&](int i, const string &key, const string &value) {
        auto &&strList = ContainerRepoItem(value).toStringList();
        for (size_t j = 0; j < strList.size(); j++)
            widths[j] = std::max(widths[j], (int)strList[j].size());
        lines.emplace_back(strList);
    });
    // output tag
    for (size_t i = 0; i < widths.size(); i++) {
        widths[i] += 2;
        out << std::left << std::setw(widths[i]) << ContainerRepoItem::ATTR_TAG_LIST[i];
    }
    out << std::endl;
    // output all value
    for (size_t i = 0; i < lines.size(); i++) {
        for (size_t j = 0; j < lines[i].size(); j++)
            out << std::left << std::setw(widths[j]) << lines[i][j];
        out << std::endl;
    }
    return out;
}

#endif // CONTAINER_REPO_CPP
