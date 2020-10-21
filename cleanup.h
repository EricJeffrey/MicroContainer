#if !defined(CLEANUP)
#define CLEANUP

#include <string>

using std::string;

/**
 * @brief cleanup a stopped container
 * @param contID id of container, cannot be name
 */
void cleanup(const string &contID) noexcept;

#endif // CLEANUP
