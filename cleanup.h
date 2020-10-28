#if !defined(CLEANUP)
#define CLEANUP

#include <string>

using std::string;

/**
 * @brief cleanup a stopped container
 * @param contID id of container, cannot be name
 * @param force force stop if container running, default false
 */
void cleanup(const string &contID, bool force = false) noexcept;

#endif // CLEANUP
