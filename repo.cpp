#if !defined(REPO_CPP)
#define REPO_CPP

#include "repo.h"

#include <memory>

/**
 * @brief add item to db
 * @exception DBError
 */
void Repo::add(const string &id, const RepoItem &item) {
    checkAndThrow();
    auto status = db->Put(leveldb::WriteOptions(), id, item.toDBString());
    if (!status.ok())
        throw DBError("add item to database failed: " + status.ToString());
}

/**
 * @brief get value of id
 * @exception DBError
 */
string Repo::get(const string &id) {
    checkAndThrow();
    string value;
    auto status = db->Get(leveldb::ReadOptions(), id, &value);
    if (!status.ok())
        throw DBError("read value from db failed: " + status.ToString());
    return value;
}

/**
 * @brief remove from db
 * @exception DBError
 */
void Repo::remove(const string &id) {
    checkAndThrow();
    auto status = db->Delete(leveldb::WriteOptions(), id);
    if (!status.ok() && !status.IsNotFound())
        throw DBError("remove item from database failed: " + status.ToString());
}

/**
 * @brief update item, add to db if not exist
 * @return old value(str) stored in db
 * @exception DBError
 */
string Repo::update(const string &id, const RepoItem &newItem) {
    checkAndThrow();
    string oldValue;
    auto status = db->Get(leveldb::ReadOptions(), id, &oldValue);
    if (!status.ok() && !status.IsNotFound())
        throw DBError("update item failed, cannot get old data: " + status.ToString());
    if (!(db->Put(leveldb::WriteOptions(), id, newItem.toDBString())).ok())
        throw DBError("update item failed, cannot put new data: " + status.ToString());
    return oldValue;
}

/**
 * @brief check if id in db
 * @exception DBError
 */
bool Repo::contains(const string &id) {
    checkAndThrow();
    string value;
    auto status = db->Get(leveldb::ReadOptions(), id, &value);
    if (status.ok())
        return true;
    else if (status.IsNotFound())
        return false;
    throw DBError("get id failed: " + status.ToString());
}

void Repo::foreach (const RepoIterFunc &job) const {
    std::unique_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));
    int index = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        job(index, it->key().ToString(), it->value().ToString());
        index++;
    }
}

#endif // REPO_CPP
