#if !defined(EXTRACH_H)
#define EXTRACH_H

#include <archive.h>
#include <archive_entry.h>

#include <string>

using std::string;

int copy_data(struct archive *ar, struct archive *aw);

// FIXME hard link should path not right
// Extract file to dirPath, dirPath should end with '/'
// Return <errorCode, msg>
std::pair<int, string> extract(const string &filePath, const string &dirPath);

#endif // EXTRACH_H
