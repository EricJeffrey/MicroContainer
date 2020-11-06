#if !defined(IMAGE_REPO_CPP)
#define IMAGE_REPO_CPP

#include <iomanip>
#include <optional>

#include "image_repo.h"
#include "utils.h"

ImageRepoItem ImageRepo::getItem(const string &name, const string &tag) {
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH());
    return getItem(idRepo.get(name + ":" + tag));
}

void ImageRepo::addImg(const string &id, const ImageRepoItem &item) {
    Repo::add(id, item);
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH());
    idRepo.add(item.name, item.tag, id);
}

void ImageRepo::removeImg(const string &name, const string &tag) {
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH());
    string id = idRepo.get(name + ":" + tag);
    idRepo.removeItem(id, name + ":" + tag);
    Repo::remove(id);
}

void ImageRepo::removeImg(const string &id) {
    Repo::remove(id);
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH());
    idRepo.removeItem(id);
}

bool ImageRepo::contains(const string &name, const string &tag) {
    ImgNameIDRepo idRepo;
    idRepo.open(IMG_ID_REPO_DB_PATH());
    return idRepo.contains(name + ":" + tag);
}

void ImageRepo::updateUsedContNum(const string &id, int value) {
    auto oldItem = getItem(id);
    oldItem.setUsedContNum(oldItem.usedContNum + value);
    update(id, oldItem);
}

std::ostream &operator<<(std::ostream &out, const ImageRepo &repo) {
    vector<vector<string>> lines;
    lines.emplace_back(ImageRepoItem::ATTR_TAG_LIST);
    repo.foreach ([&](int i, const string &key, const string &value) {
        lines.emplace_back(ImageRepoItem(value).toStringList());
        return false;
    });
    lineupPrint(out, lines);
    return out;
}

std::optional<ImageRepoItem> imageExist(const string &image) {
    ImageRepo repo;
    repo.open(IMAGE_REPO_DB_PATH());
    if (auto colonPos = image.find(':'); colonPos != string::npos) {
        if (repo.contains(image.substr(0, colonPos), image.substr(colonPos + 1)))
            return repo.getItem(image.substr(0, colonPos), image.substr(colonPos + 1));
        return std::nullopt;
    } else {
        ImageRepoItem item;
        bool possibleId = (image.size() == ID_ABBR_LENGTH || image.size() == ID_TOTAL_LEGNTH);
        repo.foreach ([&possibleId, &item, &image](int i, const string &key, const string &val) {
            ImageRepoItem tmpItem(val);
            if ((image.size() == ID_TOTAL_LEGNTH && tmpItem.imageID == image) ||
                (image.size() == ID_ABBR_LENGTH &&
                 tmpItem.imageID.substr(0, ID_ABBR_LENGTH) == image)) {
                item = tmpItem;
                return true;
            }
            return false;
        });
        if (item.imageID.empty())
            return std::nullopt;
        return item;
    }
}

#endif // IMAGE_REPO_CPP
