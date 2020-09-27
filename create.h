#if !defined(CREATE_H)
#define CREATE_H

#include <string>

using std::string;

// generate OCI-runtime config.json
void generateSpecConf(const string &dirPath);

// [imageName] can be either name:tag(default latest) or id
void createContainer(const string &name, const string &imageName) noexcept;

#endif // CREATE_H
