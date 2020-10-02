
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <thread>

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>

using std::thread;

const char attachSockPath[] = "/home/eric/coding/MicroContainer/feasibility/attach_test/attach";

void errExit(const char *msg, const char *desc = nullptr) {
    const int bufsz = strlen(msg) + (desc == nullptr ? 0 : strlen(desc)) + 25;
    char buf[bufsz] = {};
    snprintf(buf, bufsz - 1, "call to %s failed: %s", msg, desc);
    perror(buf);
    exit(1);
}

#define LOGGER(A) (fprintf(stderr, "DEBUG\t%s\n", A))

void attacherWork() {
    int sockFd = 0;
    {
        sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockFd == -1)
            errExit("socket");
        sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, attachSockPath, sizeof(addr.sun_path) - 1);
        if (connect(sockFd, (sockaddr *)&addr, sizeof(addr)) == -1)
            errExit("connect");
    }
    LOGGER("connected to container, input anything");
    thread(
        [](int sd) {
            while (true) {
                const int bufsz = 1024;
                char buf[bufsz + 1] = {};
                ssize_t numRead = read(sd, buf, bufsz);
                if (numRead == -1)
                    errExit("read");
                fprintf(stderr, "%s\n", buf);
            }
        },
        sockFd)
        .detach();
    const int bufsz = 1024;
    char buf[bufsz + 1] = {};
    while (true) {
        if (scanf("%s", buf) == -1)
            errExit("scanf eof");
        int numScan = strnlen(buf, bufsz - 2);
        fprintf(stderr, "Data scanf return: %s, num: %d\n", buf, numScan);
        buf[numScan] = '\n';
        numScan += 1;
        if (write(sockFd, buf, numScan) == -1)
            errExit("write to sock fd");
    }
}

int main(int argc, char const *argv[]) {
    attacherWork();
    return 0;
}
