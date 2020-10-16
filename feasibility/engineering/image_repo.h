#if !defined(IMAGE_REPO_H)
#define IMAGE_REPO_H

#include "config.h"
#include "image_repo_item.h"
#include "leveldb/db.h"
#include "repo.h"

#include <ostream>

struct ImgNameIDRepoItem : public RepoItem {
    const string value;
    ImgNameIDRepoItem(const string &value) : value(value) {}
    string toDBString() const override { return value; }
};
struct ImgNameIDRepo : public Repo {
    ImgNameIDRepo() {}
    ~ImgNameIDRepo() {}
    void add(const string &name, const string &tag, const string &id) {
        Repo::add(name + ":" + tag, ImgNameIDRepoItem(id));
        Repo::add(id, ImgNameIDRepoItem(name + ":" + tag));
    }
    void removeItem(const string &id) { removeItem(id, get(id)); }
    void removeItem(const string &id, const string &nametag) {
        Repo::remove(id);
        Repo::remove(nametag);
    }
};
/**
 * local image repository.
 * note: to add/remove image, use addImg(),removeImg() instead of add(),remove()
 */
class ImageRepo : public Repo {
private:
public:
    ImageRepo() : Repo() {}
    ~ImageRepo() {}

    ImageRepoItem getItem(const string &id) { return ImageRepoItem(get(id)); }
    ImageRepoItem getItem(const string &name, const string &tag);
    void addImg(const string &id, const ImageRepoItem &item);
    void removeImg(const string &id);
    void removeImg(const string &name, const string &tag);
    bool contains(const string &name, const string &tag);
    bool contains(const string &id) { return Repo::contains(id); }

    /**
     * @brief increase usedContNum of image with newValue
     * @param value value to increase, default is 1, can be negative
     */
    void updateUsedContNum(const string &id, int value = 1);
};

std::ostream &operator<<(std::ostream &o, const ImageRepo &repo);

#endif // IMAGE_REPO_H