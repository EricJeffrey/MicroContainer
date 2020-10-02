#include <set>
#include <thread>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/epoll.h>

using std::set;
using std::thread;

const char outFilePath[] = "/home/eric/coding/MicroContainer/feasibility/attach_test/foo.out";
const char attachSockPath[] = "/home/eric/coding/MicroContainer/feasibility/attach_test/attach";
const char bundlePath[] = "/home/eric/coding/MicroContainer/feasibility/attach_test/bundle";

void errExit(const char *msg, const char *desc = nullptr) {
    const int bufsz = strlen(msg) + (desc == nullptr ? 0 : strlen(desc)) + 25;
    char buf[bufsz] = {};
    snprintf(buf, bufsz - 1, "call to %s failed: %s", msg, desc);
    perror(buf);
    exit(1);
}

#define LOGGER(A) (fprintf(stderr, "DEBUG\t%s\n", A))

// call exec, no return
void childJob(int rpipe[], int wpipe[]) {
    if (close(rpipe[1]) == -1)
        errExit("close on rpipe-1");
    if (close(wpipe[0]) == -1)
        errExit("close on wpipe-0");
    LOGGER("CHILD: pipe closed");
    LOGGER("CHILD: duplicating stdio");
    if (dup2(rpipe[0], STDIN_FILENO) == -1)
        errExit("dup2 rpipe[0]->stdin");
    if (dup2(wpipe[1], STDOUT_FILENO) == -1)
        errExit("dup2 wpipe[1]->stdout");
    if (dup2(wpipe[1], STDERR_FILENO) == -1)
        errExit("dup2 wpipe[1]->stderr");
    if (execlp("crun", "crun", "run", "-d", "-b", bundlePath, "hello", nullptr) == -1)
        errExit("execlp");
    fprintf(stderr, "execlp doesn't work\n");
    exit(1);
}

// g++ -g -Wall -lpthread -o starter.out starter.cpp
void work() {
    int rpipe[2], wpipe[2];
    if (pipe(rpipe) == -1)
        errExit("pipe on read");
    if (pipe(wpipe) == -1)
        errExit("pipe on write");
    LOGGER("pipe created");
    pid_t childPid = fork();
    if (childPid == -1)
        errExit("fork");
    else if (childPid == 0) {
        // child
        childJob(rpipe, wpipe);
    } else {
        // parent
        { // close pipe end
            if (close(rpipe[0]) == -1)
                errExit("close on rpipe-0");
            if (close(wpipe[1]) == -1)
                errExit("close on wpipe-1");
        }
        LOGGER("PARENT: pipe closed");
        int outFd = 0;
        { // oepn output file
            outFd = open(outFilePath, O_WRONLY | O_CREAT,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            if (outFd == -1)
                errExit("open on outFilePath");
        }
        LOGGER("PARENT: output file created");
        int sockFd = 0;
        { // create unix socket
            sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
            sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, attachSockPath, sizeof(addr.sun_path) - 1);
            if (sockFd == -1)
                errExit("socket of unix");
            if (bind(sockFd, (sockaddr *)&addr, sizeof(addr)) == -1)
                errExit("bind on unix socket");
            if (listen(sockFd, 1024) == -1)
                errExit("listen on unix socket");
        }
        LOGGER("PARENT: unix sock created");
        int epFd = 0;
        { // init poll
            epFd = epoll_create1(0);
            if (epFd == -1)
                errExit("epoll_create1");
            epoll_event tmpEvent;
            memset(&tmpEvent, 0, sizeof(tmpEvent));
            tmpEvent.data.fd = sockFd;
            tmpEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
            if (epoll_ctl(epFd, EPOLL_CTL_ADD, sockFd, &tmpEvent) == -1)
                errExit("epoll_ctl-add unix sock");
            tmpEvent.data.fd = wpipe[0];
            tmpEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
            if (epoll_ctl(epFd, EPOLL_CTL_ADD, wpipe[0], &tmpEvent) == -1)
                errExit("epoll_ctl-add wpipe[0]");
        }
        LOGGER("PARENT: epoll created");
        set<int> outputFdSet, attachFdSet;
        { outputFdSet.insert(outFd); }
        const int event_list_size = 100;
        epoll_event eventList[event_list_size];
        while (true) {
            int eventNum = epoll_wait(epFd, eventList, event_list_size, -1);
            if (eventNum < 0) {
                errExit("epoll_wait");
            } else if (eventNum > 0) {
                for (int i = 0; i < eventNum; i++) {
                    const epoll_event event = eventList[i];
                    const int eventFd = event.data.fd;
                    // sock
                    if (eventFd == sockFd) {
                        LOGGER("PARENT: event on sock");
                        if (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                            errExit("epollwait", "EPOLLERR/HUP on unix socket");
                        else {
                            // accept and add to epoll&attachFd
                            sockaddr_un tmpAddr;
                            socklen_t len;
                            int attacherFd = accept(sockFd, (sockaddr *)&tmpAddr, &len);
                            if (attacherFd == -1)
                                errExit("accept on unix sock");
                            epoll_event tmpEvent;
                            memset(&tmpEvent, 0, sizeof(tmpEvent));
                            tmpEvent.data.fd = attacherFd;
                            tmpEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
                            if (epoll_ctl(epFd, EPOLL_CTL_ADD, attacherFd, &tmpEvent) == -1)
                                errExit("epoll_ctl on attcherFd");
                            attachFdSet.insert(attacherFd);
                            outputFdSet.insert(attacherFd);
                        }
                    }
                    // pipe
                    else if (eventFd == wpipe[0]) {
                        LOGGER("PARENT: event on wpipe");
                        if (event.events & EPOLLHUP) {
                            fprintf(stderr, "EPOLLHUP on wpipe[0], contianer process exited\n");
                            return;
                        }
                        if (event.events & EPOLLIN) {
                            LOGGER("PARENT: redirect wpipe[0] -> outputFdSet");
                            // redirect to output file and attachers
                            const int bufsz = 1024;
                            char buf[bufsz + 1] = {};
                            ssize_t numRead = read(wpipe[0], buf, bufsz);
                            if (numRead == -1)
                                errExit("read on wpipe[0]");
                            fprintf(stderr, "Data from wpipe[0]: %s\n", buf);
                            for (auto &&fd : outputFdSet)
                                if (write(fd, buf, numRead) == -1)
                                    errExit("write on output fd");
                        } else {
                            fprintf(stderr, "unknown events: %u\n", event.events);
                            return;
                        }
                    }
                    // attachers
                    else if (attachFdSet.find(eventFd) != attachFdSet.end()) {
                        LOGGER("PARENT: event on attacher");
                        if (event.events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
                            fprintf(stderr, "EPOLLERR/HUP on attacher\n");
                            if (epoll_ctl(epFd, EPOLL_CTL_DEL, eventFd, nullptr) == -1)
                                errExit("epoll_ctl on attacher fd");
                            outputFdSet.erase(eventFd);
                            attachFdSet.erase(eventFd);
                        }
                        if (event.events & EPOLLIN) {
                            LOGGER("PARENT: redirect attacher fd --> rpipe[1]");
                            const int bufsz = 1024;
                            char buf[bufsz + 1] = {};
                            ssize_t numRead = read(eventFd, buf, bufsz);
                            if (numRead == -1)
                                errExit("read on attach fd");
                            fprintf(stderr, "PARENT: attacher data: %s\n", buf);
                            if (write(rpipe[1], buf, numRead) == -1)
                                errExit("write to rpipe[1]");
                        } else {
                            fprintf(stderr, "unknown events: %u\n", event.events);
                            return;
                        }
                    }
                    // no idea what happend
                    else {
                        fprintf(stderr, "unknown event fd: %d\n", eventFd);
                    }
                }
            } else
                ; // no event
        }
    }
}

int main(int argc, char const *argv[]) { work(); }
