#if !defined(TEST_CONT_NET_H)
#define TEST_CONT_NET_H

#include "../lib/logger.h"
#include "../network.h"
#include "../utils.h"
#include "test_utils.h"

#include <iostream>

using std::cerr, std::endl;

void ip_link_addr_show() {
    cerr << "-----ip addr show-----" << endl;
    fork_exec_wait(ipPath, {ipCmd, "addr", "show"}, false);
}

void test_container_net() {
    Logger::init(std::cerr);
    if (getuid() != 0) {
        cerr << "THIS TEST NEED TO RUN AS ROOT" << endl;
        return;
    }
    createBridge();

    const string containerID = "slkdghoshglksadnglkasjssldk";
    const string ip = "10.66.1.43";
    const string gateway = ip.substr(0, ip.find_last_of('.')) + ".1";
    const string netnsName = NET_NS_NAME_PREFIX + containerID.substr(0, 8);

    cerr << "-----TESTING CREATE CONTAINER NETWORK-----" << endl;
    auto [err, veth1, veth2] = createContNet(containerID, ip, 24);

    fork_exec_wait("/usr/sbin/ping", {"ping", "-c", "4", ip}, false);
    fork_exec_wait(ipPath, {ipCmd, "netns", "exec", netnsName, "ping", "-c", "4", gateway}, false);
    cerr << "-----ip addr show-----" << endl;
    fork_exec_wait(ipPath, {ipCmd, "addr", "show"}, false);
    cerr << "-----ip addr ns show-----" << endl;
    fork_exec_wait(ipPath, {ipCmd, "netns", "exec", netnsName, "ip", "addr", "show"}, false);
    anykey();

    cerr << "-----TESTING CLEANUP CONTAINER NETWORK-----" << endl;
    cleanupContNet(containerID);
    cerr << "-----ip addr show-----" << endl;
    fork_exec_wait(ipPath, {ipCmd, "addr", "show"}, false);
}

#endif // TEST_CONT_NET_H
