#if !defined(STOP_H)
#define STOP_H

#include <string>

using std::string;

/**
 * @brief stop container
 * @param ignoreKill ignore non-existence of container process if true
 * @return true on success
 */
bool stop(const string &container, bool ignoreKill = false) noexcept;

#endif // STOP_H
