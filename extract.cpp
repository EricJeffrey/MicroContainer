#if !defined(EXTRACT_CPP)
#define EXTRACT_CPP

#include "extract.h"

#include <stdexcept>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

int copy_data(struct archive *ar, struct archive *aw) {
    int ret;
    const void *buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
        ret = archive_read_data_block(ar, &buff, &size, &offset);
        if (ret == ARCHIVE_EOF)
            return ARCHIVE_OK;
        if (ret < ARCHIVE_OK)
            return ret;
        ret = archive_write_data_block(aw, buff, size, offset);
        if (ret < ARCHIVE_OK)
            return ret;
    }
}

// Extract file to dirPath, dirPath should end with '/'
// return -2 with warn, throw on error
std::pair<int, string> extract(const string &filePath, const string &dirPath) {
    using std::runtime_error;
    struct ArchiveGuard {
        struct archive *res;
        bool typeRead;
        ArchiveGuard(struct archive *a, bool typeRead) : res(a), typeRead(typeRead) {}
        ~ArchiveGuard() {
            if (res != nullptr)
                typeRead ? archive_read_free(res) : archive_write_free(res);
        }
    };

    struct archive *src, *dst;
    struct archive_entry *entry;
    int flags, ret;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    src = archive_read_new();
    if (src == nullptr)
        throw runtime_error("call to archive_read_new failed");
    ArchiveGuard guardSrc(src, true);

    archive_read_support_format_all(src);
    archive_read_support_filter_all(src);
    dst = archive_write_disk_new();
    if (dst == nullptr)
        throw runtime_error("call to archive_write_disk_new failed");
    ArchiveGuard guardDst(dst, false);

    archive_write_disk_set_options(dst, ARCHIVE_EXTRACT_TIME);
    archive_write_disk_set_standard_lookup(dst);
    ret = archive_read_open_filename(src, filePath.c_str(), 10240);
    if (ret != ARCHIVE_OK)
        throw runtime_error(string("call to archive_read_open_filename failed:") +
                            archive_error_string(src));
    std::pair<int, string> res(0, "");
    auto setResWarn = [&](const string &msg) { res.first = -2, res.second += msg + ", "; };
    for (;;) {
        ret = archive_read_next_header(src, &entry);
        if (ret == ARCHIVE_EOF)
            break;
        if (ret < ARCHIVE_WARN)
            throw runtime_error(archive_error_string(src));
        if (ret < ARCHIVE_OK) {
            setResWarn(archive_error_string(src));
        }

        const char *pathName = archive_entry_pathname(entry);
        if (pathName == nullptr)
            throw runtime_error("call to archive_entry_pathname failed");

        const string fullPathName = dirPath + archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, fullPathName.c_str());

        ret = archive_write_header(dst, entry);
        if (ret < ARCHIVE_OK)
            setResWarn(archive_error_string(dst));
        else if (archive_entry_size(entry) > 0) {
            ret = copy_data(src, dst);
            if (ret < ARCHIVE_WARN)
                throw runtime_error(archive_error_string(dst));
            else if (ret < ARCHIVE_OK)
                setResWarn(archive_error_string(dst));
        }
        ret = archive_write_finish_entry(dst);
        if (ret < ARCHIVE_WARN)
            throw runtime_error(archive_error_string(dst));
        else if (ret < ARCHIVE_OK)
            setResWarn(archive_error_string(dst));
    }
    return res;
}

#endif // EXTRACT_CPP
