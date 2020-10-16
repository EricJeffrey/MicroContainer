#if !defined(REPO_H)
#define REPO_H

#include "db_error.h"
#include "leveldb/db.h"
#include "repo_item.h"

#include <functional>
#include <iostream>
#include <string>
using std::string;

typedef std::function<void(int, const string &, const string &)> RepoIterFunc;

/**
 * @brief Provide basic RAII levelDB database manipulate func
 */
class Repo {
protected:
    leveldb::DB *db;

    /**
     * @brief check is database has been opened
     * @exception DBError
     */
    void checkAndThrow() {
        if (db == nullptr)
            throw DBError("database not open");
    }

public:
    Repo() : db(nullptr) {}
    virtual ~Repo() {
        if (db != nullptr)
            delete db;
        db = nullptr;
    }
    // no move & copy
    Repo(const Repo &) = delete;
    Repo(const Repo &&) = delete;
    Repo &operator=(const Repo &) = delete;
    Repo &operator=(const Repo &&) = delete;

    leveldb::DB *getDBPointer() const { return db; }

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
    void foreach (const RepoIterFunc &job) const;
};

#endif // REPO_H
