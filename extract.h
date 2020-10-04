#if !defined(EXTRACH_H)
#define EXTRACH_H

#include <archive.h>
#include <archive_entry.h>

#include <string>

using std::string;

// Extract file to dirPath, dirPath should end with '/'
// Return <errorCode, msg>
std::pair<int, string> extract(const string &filePath, const string &dirPath);

#endif // EXTRACH_H
