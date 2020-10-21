#if !defined(NETWORK_CPP)
#define NETWORK_CPP

#include <bitset>
#include <filesystem>

#include "config.h"
#include "lib/logger.h"
#include "network.h"
#include "utils.h"

/**
 * Create netns of name, stored in /var/run/netns/name.
 * Using "ip netns add name" (fork & exec) to create.
 *
 * @return true if successfully create netns, otherwise false
 * @exception SysError, IncorrectlyExitError
 */
inline bool createNetns(const string &name) {
    return fork_exec_wait("/usr/sbin/ip", {"ip", "netns", "add", name}, true) == 0;
}

// throw on error
void addDefGatewayRoute(const string &gatewayIp, const string &devName) {
    vector<string> addRouteArgv(
        {ipCmd, "route", "add", "default", "via", gatewayIp, "dev", devName});
    if (fork_exec_wait(ipPath, addRouteArgv) != 0)
        throw runtime_error("Add route failed on exec: " + concat(addRouteArgv));
}

// throw on error
void addPortMap(int hostPort, const string &destIp, int destPort, const string &proto) {
    // iptables -A nat ! -i br-microc -p tcp -m tcp --dport 7880 -j DNAT --to-destination
    // 172.17.0.4:8080
    using std::to_string;
    vector<string> portMapArgv({iptablesName, "-t", "nat", "-A", "PREROUTING", "!", "-i",
                                BRIDGE_NAME, "-p", proto, "-m", proto, "--dport",
                                to_string(hostPort), "-j", "DNAT", "--to-destination",
                                destIp + ":" + to_string(destPort)});
    if (fork_exec_wait(iptablesPath, portMapArgv) != 0)
        throw runtime_error("Add port mapping failed on exec: " + concat(portMapArgv));
}

// configure nat if not exist, throw on error
void confBridgeNat() {
    vector<string> checkArgv({iptablesName, "-t", "nat", "-C", "POSTROUTING", "-s",
                              BRIDGE_SUBNET_ADDR, "!", "-d", BRIDGE_SUBNET_ADDR, "-j",
                              "MASQUERADE"});
    vector<string> addArgv({iptablesName, "-t", "nat", "-A", "POSTROUTING", "-s",
                            BRIDGE_SUBNET_ADDR, "!", "-d", BRIDGE_SUBNET_ADDR, "-j", "MASQUERADE"});
    int retCode = fork_exec_wait(iptablesPath, checkArgv);
    if (retCode == 0) {
        return;
    } else {
        retCode = fork_exec_wait(iptablesPath, addArgv);
        if (retCode != 0)
            throw runtime_error("Add iptables rule failed: " + concat(addArgv));
    }
}

// throw on error
void deleteBridgeNat() {
    vector<string> delArgv({iptablesName, "-t", "nat", "-D", "POSTROUTING", "-s",
                            BRIDGE_SUBNET_ADDR, "!", "-d", BRIDGE_SUBNET_ADDR, "-j", "MASQUERADE"});
    if (fork_exec_wait(iptablesPath, delArgv) != 0)
        throw runtime_error("Delete iptables rule failed: " + concat(delArgv));
}

// throw on error
void deleteBridgeDev() {
    vector<string> argv = {ipCmd, "link", "delete", BRIDGE_NAME};
    if (fork_exec_wait(ipPath, argv) != 0)
        throw runtime_error("Delete bridge failed on exec: " + concat(argv));
}

// throw on error
bool bridgeExits() { return fork_exec_wait(ipPath, {ipCmd, "link", "show", BRIDGE_NAME}) == 0; }

// throw on error
void createBridgeDev() {
    vector<vector<string>> argvs = {
        {ipCmd, "link", "add", BRIDGE_NAME, "type", "bridge"},
        {ipCmd, "address", "add", BRIDGE_ADDR, "dev", BRIDGE_NAME},
        {ipCmd, "link", "set", BRIDGE_NAME, "up"},
    };
    for (auto &&argv : argvs) {
        int retCode = fork_exec_wait(ipPath, argv, false);
        if (retCode != 0)
            throw runtime_error("Bridge create failed on exec: " + concat(argv));
    }
}

bool createBridge() noexcept {
    try {
        if (!bridgeExits()) {
            createBridgeDev();
            confBridgeNat();
        }
        return true;
    } catch (const std::exception &e) {
        loggerInstance()->error("Create bridge failed:", e.what());
        try {
            if (bridgeExits()) {
                deleteBridgeDev();
                deleteBridgeNat();
            }
        } catch (const std::exception &e) {
            loggerInstance()->error("Clean device failed:", e.what());
        }
    }
    return false;
}

std::tuple<bool, string, string> createContNet(const string &contId, const string &ip,
                                               int ipPreLen) noexcept {
    const string nsName = NET_NS_PATH_PREFIX + contId.substr(0, NET_DEV_NS_NAME_SUFFIX_LEN);
    const string veth1 = "veth" + genRandomStr(NET_DEV_NS_NAME_SUFFIX_LEN);
    const string veth2 = "veth" + genRandomStr(NET_DEV_NS_NAME_SUFFIX_LEN);
    const string gateway = ip.substr(0, ip.find_last_of('.')) + ".1";
    const bool err = true;
    try {
        vector<vector<string>> cmds = {
            {ipCmd, "netns", "add", nsName},
            {ipCmd, "link", "add", veth1, "type", "veth", "peer", "name", veth2},
            {ipCmd, "link", "set", veth1, "master", BRIDGE_NAME},
            {ipCmd, "link", "set", veth1, "up"},
            {ipCmd, "link", "set", veth2, "netns", nsName},
            {ipCmd, "netns", "exec", nsName, ipCmd, "address", "add",
             ip + "/" + std::to_string(ipPreLen), "dev", veth2},
            {ipCmd, "netns", "exec", nsName, ipCmd, "link", "set", "lo", "up"},
            {ipCmd, "netns", "exec", nsName, ipCmd, "link", "set", veth2, "up"},
            {ipCmd, "netns", "exec", nsName, ipCmd, "route", "add", "default", "via", gateway,
             "dev", veth2},
        };
        for (auto &&cmd : cmds) {
            if (fork_exec_wait(ipPath, cmd) != 0) {
                loggerInstance()->error("Configure container network faild while exec:",
                                        concat(cmd));
                return {err, veth1, veth2};
            }
        }
        return {!err, veth1, veth2};
    } catch (const std::exception &e) {
        loggerInstance()->error("Create container network failed:", e.what());
    } catch (...) {
        loggerInstance()->error("Create container network failed, unknown exception");
    }
    return {err, veth1, veth2};
}

void cleanupContNet(const string &contId) noexcept {
    const string nsName = NET_NS_PATH_PREFIX + contId.substr(0, NET_DEV_NS_NAME_SUFFIX_LEN);
    try {
        fork_exec_wait(ipPath, {ipCmd, "netns", "del", nsName});
        fork_exec_wait("/usr/bin/umount", {"umount", "/run/netns/" + nsName});
        fork_exec_wait("rm", {"/run/netns/" + nsName});
    } catch (const std::exception &e) {
        loggerInstance()->error("Cleanup container network failed:", e.what());
    }
}

string IpRepo::useOne() {
    using std::bitset;
    checkAndThrow();
    constexpr int ipaddrNum = (1 << VETH_IP_PREFIX_LEN);
    bitset<ipaddrNum> used;
    string value;
    auto status = db->Get(leveldb::ReadOptions(), IP_REPO_KEY, &value);
    if (status.ok())
        used = bitset<ipaddrNum>(value);
    else if (!status.IsNotFound())
        throw DBError("Failed to read ip address repo:" + status.ToString());
    // only check high/low 8 bit, may fail if VETH_IP_PREFIX_LEN not 16
    for (size_t addr = 0; addr < used.size(); addr++) {
        // cannot use .0.* or .255.* or .*.0 or .*.255
        if ((addr & 0xff) == 0 || (addr & 0xff) == 0xff)
            continue;
        if (((addr >> 8) & 0xff) == 0 || ((addr >> 8) & 0xff) == 0xff)
            continue;
        if (!used[addr]) {
            used.set(addr);
            auto sta = db->Put(leveldb::WriteOptions(), IP_REPO_KEY, used.to_string());
            if (!sta.ok())
                throw DBError("Failed to update ip address repo:" + sta.ToString());
            return string(VETH_IP_PREFIX) + "." + std::to_string(addr & 0xff) + "." +
                   std::to_string((addr >> 8) & 0xff);
        }
    }
    throw NoAvailIPError("No availiable ip in repo");
}

#endif // NETWORK_CPP
