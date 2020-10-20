#if !defined(NETWORK_H)
#define NETWORK_H

#include <string>

using std::string;

static const string BRIDGE_NAME = "br-microc";
static const string BRIDGE_ADDR = "10.66.1.1/24";
static const string BRIDGE_SUBNET_ADDR = "10.66.1.0/24";

// length of random string of name of net device, e.g. microc-alkij6kn
static const size_t NET_DEV_NAME_SUFFIX_LEN = 8;

static const char ipPath[] = "/usr/sbin/ip";
static const char ipCmd[] = "ip";
static const char iptablesPath[] = "/usr/sbin/iptables";
static const char iptablesName[] = "iptables";

static const string NET_NS_PATH_PREFIX = "microc-";

// throw on error
void addPortMap(int hostPort, const string &destIp, int destPort, const string &proto = "tcp");

// create bridge and config nat (via iptable)
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
 * Note: this function won't throw(and log) when return value of fork_exec_wait is not 0 
 */
void cleanupContNet(const string &contId, const std::pair<string, string> &vethPair) noexcept;

#endif // NETWORK_H
