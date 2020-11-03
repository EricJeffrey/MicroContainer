#if !defined(RUN_H)
#define RUN_H

#include <string>
#include <vector>

using std::string, std::vector;

void runContainer(const string &image, const vector<string> &args, const string &name,
                  bool doAttach) noexcept;

#endif // RUN_H
