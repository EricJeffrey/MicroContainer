#if !defined(NETWORK_H)
#define NETWORK_H

#include "utils.h"
#include "lib/logger.h"

const string bridgeName = "br-microc";
const string bridgeAddr = "10.0.1.1/24";
const string bridgeSubnetAddr = "10.0.1.0/24";

static const char ipPath[] = "/usr/sbin/ip";
static const char ipName[] = "ip";
static const char iptablesPath[] = "/usr/sbin/iptables";
static const char iptablesName[] = "iptables";

static const string netNsPathPrefix = "microc-";

// throw on error
void addDefGatewayRoute(const string &gatewayIp, const string &devName);

// throw on error
void addPortMap(int hostPort, const string &destIp, int destPort, const string &proto = "tcp");

// throw on error
void createVethPeer(const string &vethName1, const string &vethName2);

// throw on error
void addVethToNetns(const string &vethName, pid_t pid);

// throw on error
void addVethToBridge(const string &vethName);

// throw on error
void confVethIp(const string &vethName, const string &ipAddr, int ipPreLen);

// throw on error
void confVethUp(const string &vethName);

// configure nat if not exist, throw on error
void confBridgeNat();

// throw on error
void deleteBridgeNat();

// throw on error
void deleteBridgeDev();

// throw on error
bool checkBridgeExits();

// throw on error
void createBridgeDev();

// create bridge and config nat (via iptable)
// no throw
bool createBridgeUpIP();

#endif // NETWORK_H
