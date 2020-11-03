#if !defined(CREATE_H)
#define CREATE_H

#include <string>
#include <vector>

using std::string, std::vector;

// generate OCI-runtime config.json
void generateSpecConf(const string &dirPath);

// [imageName] can be either name:tag(default latest) or id
void createContainer(const string &imgName, const string &name) noexcept;

void createContainer(const string &imgName, const string &name,
                     const vector<string> &args) noexcept;

#endif // CREATE_H
