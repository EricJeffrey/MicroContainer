#if !defined(REPO_H)
#define REPO_H

#include "db_error.h"
#include "leveldb/db.h"
#include "repo_item.h"

#include <functional>
#include <iostream>
#include <string>
using std::string;

typedef std::function<bool(int, const string &, const string &)> RepoIterFunc;

// Basic RAII levelDB database manager
class Repo {
protected:
    leveldb::DB *db;

    /**
     * @brief check is database has been opened
     * @exception DBError
     */
    inline void checkAndThrow() {
        if (db == nullptr)
            throw DBError("database not open");
    }

public:
    Repo() : db(nullptr) {}
    virtual ~Repo() {
        if (db != nullptr)
            delete db;
    }
    // no move & copy
    Repo(const Repo &) = delete;
    Repo(const Repo &&) = delete;
    Repo &operator=(const Repo &) = delete;
    Repo &operator=(const Repo &&) = delete;

    /**
     * @brief open database at path
     * @param create if true, create database when path not found
     * @exception DBError
     */
    void open(const string &path, bool create = true) {
        leveldb::Options options;
        options.create_if_missing = create;
        auto status = leveldb::DB::Open(options, path, &db);
        if (!status.ok())
            throw DBError("open database: " + path + " failed: " + status.ToString());
    }
    void close() {
        if (db != nullptr)
            delete db;
        db = nullptr;
    }
    /**
     * @brief add item to db
     * @exception DBError
     */
    void add(const string &id, const RepoItem &item);
    /**
     * @brief get value of id
     * @exception DBError
     */
    string get(const string &id);
    /**
     * @brief remove from db
     * @exception DBError
     */
    void remove(const string &id);
    /**
     * @brief update item, add to db if not exist
     * @return old value(str) stored in db
     * @exception DBError
     */
    string update(const string &id, const RepoItem &newItem);
    /**
     * @brief check if id in db
     * @exception DBError
     */
    bool contains(const string &id);
    /**
     * @brief Iterate a repo, stop iterate if job return true
     * @param job function object: bool(int index, const string &key, const string& value)
     */
    void foreach (const RepoIterFunc &job) const;
};

#endif // REPO_H
