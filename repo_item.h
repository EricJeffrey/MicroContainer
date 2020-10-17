#if !defined(REPO_ITEM_H)
#define REPO_ITEM_H

#include <string>
#include <vector>
using std::string, std::vector;

class RepoItem {
public:
    RepoItem() = default;
    virtual ~RepoItem() {}
    virtual string toDBString() const = 0;
};

#endif // REPO_ITEM_H
