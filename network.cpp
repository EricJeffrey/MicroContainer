#if !defined(NETWORK_CPP)
#define NETWORK_CPP

#include "network.h"

// throw on error
void addDefGatewayRoute(const string &gatewayIp, const string &devName) {
    vector<string> addRouteArgv(
        {ipName, "route", "add", "default", "via", gatewayIp, "dev", devName});
    if (fork_exec_wait(ipPath, addRouteArgv) != 0)
        throw runtime_error("Add route failed on exec: " + concat(addRouteArgv));
}

// throw on error
void addPortMap(int hostPort, const string &destIp, int destPort, const string &proto) {
    // iptables -A nat ! -i br-microc -p tcp -m tcp --dport 7880 -j DNAT --to-destination
    // 172.17.0.4:8080
    using std::to_string;
    vector<string> portMapArgv({iptablesName, "-t", "nat", "-A", "PREROUTING", "!", "-i",
                                bridgeName, "-p", proto, "-m", proto, "--dport",
                                to_string(hostPort), "-j", "DNAT", "--to-destination",
                                destIp + ":" + to_string(destPort)});
    if (fork_exec_wait(iptablesPath, portMapArgv) != 0)
        throw runtime_error("Add port mapping failed on exec: " + concat(portMapArgv));
}

// throw on error
void createVethPeer(const string &vethName1, const string &vethName2) {
    vector<string> addVethArgv(
        {ipName, "link", "add", vethName1, "type", "veth", "peer", "name", vethName2});
    if (fork_exec_wait(ipPath, addVethArgv) != 0)
        throw runtime_error("Add veth peer failed on exec: " + concat(addVethArgv));
}

// throw on error
void addVethToNetns(const string &vethName, pid_t pid) {
    vector<string> addToNsArgv({ipName, "link", "set", vethName, "netns", std::to_string(pid)});
    if (fork_exec_wait(ipPath, addToNsArgv) != 0)
        throw runtime_error("Add veth to netns failed on exec: " + concat(addToNsArgv));
}

// throw on error
void addVethToBridge(const string &vethName) {
    vector<string> addVeth2BrArgv({ipName, "link", "set", vethName, "master", bridgeName});
    if (fork_exec_wait(ipPath, addVeth2BrArgv) != 0)
        throw runtime_error("Add veth to bridge failed on exec: " + concat(addVeth2BrArgv));
}

// throw on error
void confVethIp(const string &vethName, const string &ipAddr, int ipPreLen) {
    // should be called inside netns
    vector<vector<string>> argvs({
        {ipName, "address", "add", ipAddr + "/" + std::to_string(ipPreLen), "dev", vethName},
        {ipName, "link", "set", vethName, "up"},
    });
    for (auto &&argv : argvs) {
        int retCode = fork_exec_wait(ipPath, argv);
        if (retCode != 0)
            throw runtime_error("Config veth ip failed on exec: " + concat(argv));
    }
}

// throw on error
void confVethUp(const string &vethName) {
    vector<string> argv = {ipName, "link", "set", vethName, "up"};
    if (fork_exec_wait(ipPath, argv) != 0)
        throw runtime_error("Config veth up failed on exec: " + concat(argv));
}

// configure nat if not exist, throw on error
void confBridgeNat() {
    vector<string> checkArgv({iptablesName, "-t", "nat", "-C", "POSTROUTING", "-s",
                              bridgeSubnetAddr, "!", "-d", bridgeSubnetAddr, "-j", "MASQUERADE"});
    vector<string> addArgv({iptablesName, "-t", "nat", "-A", "POSTROUTING", "-s", bridgeSubnetAddr,
                            "!", "-d", bridgeSubnetAddr, "-j", "MASQUERADE"});
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
    vector<string> delArgv({iptablesName, "-t", "nat", "-D", "POSTROUTING", "-s", bridgeSubnetAddr,
                            "!", "-d", bridgeSubnetAddr, "-j", "MASQUERADE"});
    if (fork_exec_wait(iptablesPath, delArgv) != 0)
        throw runtime_error("Delete iptables rule failed: " + concat(delArgv));
}

// throw on error
void deleteBridgeDev() {
    vector<string> argv = {ipName, "link", "delete", bridgeName};
    if (fork_exec_wait(ipPath, argv) != 0)
        throw runtime_error("Delete bridge failed on exec: " + concat(argv));
}

// throw on error
bool checkBridgeExits() {
    return fork_exec_wait(ipPath, {ipName, "link", "show", bridgeName}) == 0;
}

// throw on error
void createBridgeDev() {
    vector<vector<string>> argvs = {
        {ipName, "link", "add", bridgeName, "type", "bridge"},
        {ipName, "address", "add", bridgeAddr, "dev", bridgeName},
        {ipName, "link", "set", bridgeName, "up"},
    };
    for (auto &&argv : argvs) {
        int retCode = fork_exec_wait(ipPath, argv, false);
        if (retCode != 0)
            throw runtime_error("Bridge create failed on exec: " + concat(argv));
    }
}

// FIXME what about log?

// create bridge and config nat (via iptable)
// no throw
bool createBridgeUpIP() {
    try {
        if (!checkBridgeExits()) {
            // cout << "Creating bridge" << endl;
            createBridgeDev();
            confBridgeNat();
            // cout << "Created bridge" << endl;
        }
        return true;
    } catch (const std::exception &e) {
        // cerr << "Create bridge failed: " << e.what() << endl;
        try {
            if (checkBridgeExits()) {
                // cout << "Deleting bridge" << endl;
                deleteBridgeDev();
                deleteBridgeNat();
            }
        } catch (const std::exception &e) {
            // cerr << "Delete bridge failed: " << e.what() << endl;
        }
    }
    return false;
}

#endif // NETWORK_CPP
