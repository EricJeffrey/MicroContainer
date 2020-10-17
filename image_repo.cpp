#if !defined(IMAGE_REPO_CPP)
#define IMAGE_REPO_CPP

#include <iomanip>

#include "image_repo.h"

ImageRepoItem ImageRepo::getItem(const string &name, const string &tag) {
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH);
    return getItem(idRepo.get(name + ":" + tag));
}

void ImageRepo::addImg(const string &id, const ImageRepoItem &item) {
    Repo::add(id, item);
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH);
    idRepo.add(item.name, item.tag, id);
}

void ImageRepo::removeImg(const string &name, const string &tag) {
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH);
    string id = idRepo.get(name + ":" + tag);
    idRepo.removeItem(id, name + ":" + tag);
    Repo::remove(id);
}

void ImageRepo::removeImg(const string &id) {
    Repo::remove(id);
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH);
    idRepo.remove(id);
}

bool ImageRepo::contains(const string &name, const string &tag) {
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH);
    return idRepo.contains(name + ":" + tag);
}

void ImageRepo::updateUsedContNum(const string &id, int value) {
    auto oldItem = getItem(id);
    oldItem.setUsedContNum(oldItem.usedContNum + value);
    update(id, oldItem);
}

std::ostream &operator<<(std::ostream &out, const ImageRepo &repo) {
    vector<int> widths;
    for (auto &&attr : ImageRepoItem::ATTR_TAG_LIST)
        widths.push_back(attr.size());
    vector<vector<string>> lines;
    repo.foreach ([&](int i, const string &key, const string &value) {
        auto &&strList = ImageRepoItem(value).toStringList();
        for (size_t j = 0; j < strList.size(); j++)
            widths[j] = std::max(widths[j], (int)strList[j].size());
        lines.emplace_back(strList);
    });
    // output tag
    for (size_t i = 0; i < widths.size(); i++) {
        widths[i] += 2;
        out << std::left << std::setw(widths[i]) << ImageRepoItem::ATTR_TAG_LIST[i];
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

#endif // IMAGE_REPO_CPP
