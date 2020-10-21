#if !defined(NETWORK_H)
#define NETWORK_H

#include <exception>
#include <stdexcept>
#include <string>

#include "repo.h"

using std::string;

static constexpr char BRIDGE_NAME[] = "br-microc";
static constexpr char BRIDGE_ADDR[] = "10.66.1.1/16";
static constexpr char BRIDGE_SUBNET_ADDR[] = "10.66.1.0/16";
static constexpr char VETH_IP_PREFIX[] = "10.66";
static constexpr int VETH_IP_PREFIX_LEN = 16;

// length of random string of name of net device, e.g. microc-alkij6kn
static constexpr size_t NET_DEV_NS_NAME_SUFFIX_LEN = 8;

static constexpr char ipPath[] = "/usr/sbin/ip";
static constexpr char ipCmd[] = "ip";
static constexpr char iptablesPath[] = "/usr/sbin/iptables";
static constexpr char iptablesName[] = "iptables";

static constexpr char NET_NS_PATH_PREFIX[] = "microc-";

struct NoAvailIPError : public std::runtime_error {
    NoAvailIPError(const char *msg) : std::runtime_error(msg) {}
    NoAvailIPError(const string &msg) : std::runtime_error(msg) {}
};

// IP address storage, call open before access any member
class IpRepo : public Repo {
private:
    static constexpr char IP_REPO_KEY[] = "IPBITSET";

public:
    IpRepo() {}
    ~IpRepo() {}

    /**
     * @brief Get an ip address from repo.
     * If repo has no value, init it with full 254*254 address
     * @exception NoAvailIPError DBError
     */
    string useOne();
};

// throw on error
void addPortMap(int hostPort, const string &destIp, int destPort, const string &proto = "tcp");

// create bridge(if not exist) and config nat (via iptable)
// no throw
bool createBridge() noexcept;

/**
 * Configure a netns for container to use, this will:
 * 1. Create netns(container id as part of name) and veth (auto generate name)
 * 2. Add veth1 to bridge, add veth2 to netns
 * 3. Assign ip to veth2 and set up
 * 4. Add default gateway(.1 of ip) in netns
 *
 * return {err(bool), veth1, veth2}
 */
std::tuple<bool, string, string> createContNet(const string &contId, const string &ip,
                                               int ipPreLen) noexcept;

/**
 * Cleanup network of a container, this will:
 * 1. delete netns
 * 2. remove veth1 from bridge
 * 3. delete veth1 & veth2
 *
 * Note: this function won't throw(and log) when fork_exec_wait returned with nozero value
 */
void cleanupContNet(const string &contId) noexcept;

#endif // NETWORK_H
