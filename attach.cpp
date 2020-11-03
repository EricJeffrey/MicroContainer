#if !defined(ATTACH_CPP)
#define ATTACH_CPP

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "cleanup.h"
#include "config.h"
#include "container_repo.h"
#include "lib/logger.h"
#include "sys_error.h"

termios ttyOrigin;
void ttyReset() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttyOrigin) == -1) {
        loggerInstance()->sysError(errno, "failed to reset tty attribute");
    }
}
/**
 * @brief set tty attr raw
 * @return -1 on error
 */
int ttySetRaw(int fd, struct termios *prevTermios) {
    /* [tlpi](https://www.man7.org/tlpi/code/online/dist/tty/tty_functions.c.html#ttySetRaw) */

    struct termios t;
    if (tcgetattr(fd, &t) == -1)
        return -1;
    if (prevTermios != NULL)
        *prevTermios = t;
    t.c_lflag &= ~(ICANON | ECHONL | ISIG | IEXTEN | ECHO);
    /* Noncanonical mode, disable signals, extended
       input processing, and echoing */

    t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK | ISTRIP | IXON | PARMRK);
    /* Disable special handling of CR, NL, and BREAK.
       No 8th-bit stripping or parity error handling.
       Disable START/STOP output flow control. */

    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag &= ~(CS8);

    t.c_oflag &= ~OPOST; /* Disable all output processing */

    t.c_cc[VMIN] = 1;  /* Character-at-a-time input */
    t.c_cc[VTIME] = 0; /* with blocking */
    if (tcsetattr(fd, TCSAFLUSH, &t) == -1)
        return -1;
    return 0;
}

/**
 * @brief connect to unix socket of sockPath
 * @return socket file descriptor on success
 * @exception SysError
 */
int connect2unixSock(const char *sockPath) {
    int sockFd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sockFd == -1)
        throw SysError(errno, "create unix socket failed");
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockPath, sizeof(addr.sun_path) - 1);
    if (connect(sockFd, (sockaddr *)&addr, sizeof(addr)) == -1)
        throw SysError(errno, "connect to unix socket failed");
    return sockFd;
}

/**
 * @brief redirect stdin data to sock
 * @return 1 if redirect success, 0 on ctrl+p, -1 on EOF
 * @exception SysError, runtime_error
 */
int stdinEvHandler(int sockFd, epoll_event ev) {
    if (ev.events & EPOLLERR)
        throw runtime_error("EPOLLERR on stdin");
    const size_t bufsz = 1024;
    char buf[bufsz];
    ssize_t numRead = read(STDIN_FILENO, buf, bufsz);
    if (numRead < 0)
        throw SysError(errno, "read on stdin failed");
    else if (numRead == 0) {
        return -1;
    } else {
        // ctrl+p
        if (buf[0] == '\x10') {
            loggerInstance()->info("detaching, bye");
            return 0;
        }
        if (auto tmp = write(sockFd, buf, numRead); tmp < 0)
            throw SysError(errno, "write to container socket failed");
        else if (tmp != numRead)
            throw runtime_error("failed to write all data to container socket");
        else
            return 1;
    }
};

/**
 * @brief redirect sock data to stdout/stderr
 * @return 1 if success, 0 on HUP or EOF
 * @exception SysError, runtime_error
 */
int sockEvHandler(int sockFd, epoll_event ev) {
    if (ev.events & EPOLLERR)
        throw runtime_error("EPOLLERR on sock");
    else if (ev.events & (EPOLLHUP | EPOLLRDHUP))
        return 0;
    else {
        const size_t bufsz = 1024;
        char buf[bufsz];
        ssize_t numRead = read(sockFd, buf, bufsz);
        if (numRead < 0)
            throw SysError(errno, "read container socket failed");
        else if (numRead == 0)
            return 0;
        else {
            int fd2write = (buf[0] == 2 ? STDOUT_FILENO : buf[0] == 3 ? STDERR_FILENO : -1);
            const char *fd2writeStr = (fd2write == 2 ? "stdout" : "stderr");
            // conmon use first byte to indicate stdout/stderr
            if (fd2write == -1)
                throw runtime_error("invalid response container socket");
            if (auto tmp = write(fd2write, buf + 1, numRead - 1); tmp == -1)
                throw SysError(errno, string("write to ") + fd2writeStr + " failed");
            else if (tmp < numRead - 1)
                throw runtime_error(string("failed to write all data to ") + fd2writeStr);
            else
                return 1;
        }
    }
};

void attach(const string &container) noexcept {
    int epFd = 0, sockFd = 0;
    // close fd and reset tty
    auto clean = [&epFd, &sockFd]() {
        if (epFd != 0)
            close(epFd);
        if (sockFd != 0)
            close(sockFd);
        ttyReset();
        write(STDOUT_FILENO, "\n", 1);
    };
    try {
        string containerId;
        // get container id
        {
            ContainerRepoItem targetItem;
            if (auto tmp = containerExist(container); tmp.has_value()) {
                targetItem = tmp.value();
            } else {
                loggerInstance()->info("container", container, "not found");
                return;
            }
            if (targetItem.status.contStatus != ContainerStatus::RUNNING) {
                loggerInstance()->info("container not running");
                return;
            }
            containerId = targetItem.containerID;
        }
        winsize ws;
        // set current tty
        {
            if (tcgetattr(STDIN_FILENO, &ttyOrigin) == -1)
                throw SysError(errno, "get tty attribute failed");
            if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)
                throw SysError(errno, "ioctl on stdin failed");
            // reset terminal at exit
            if (atexit(ttyReset) != 0)
                throw SysError(errno, "set atexit hook failed");
            if (ttySetRaw(STDIN_FILENO, &ttyOrigin) == -1)
                throw SysError(errno, "set tty raw failed");
        }
        sockFd = connect2unixSock((SOCK_DIR_PATH() + containerId + "/attach").c_str());
        epFd = epoll_create1(0);
        if (epFd == -1)
            throw SysError(errno, "create epoll file descriptor failed");
        // add stdin & sock to epoll
        {
            epoll_event tmpEvent;
            auto add2epoll = [&tmpEvent](int epFd, int fd, uint32_t events) {
                memset(&tmpEvent, 0, sizeof(tmpEvent));
                tmpEvent.data.fd = fd, tmpEvent.events = events;
                if (epoll_ctl(epFd, EPOLL_CTL_ADD, fd, &tmpEvent) == -1)
                    throw SysError(errno, "add fd to epoll failed");
            };
            add2epoll(epFd, STDIN_FILENO, EPOLLIN | EPOLLHUP);
            add2epoll(epFd, sockFd, EPOLLIN | EPOLLRDHUP | EPOLLHUP);
        }

        loggerInstance()->info("attached to container, use ctrl+p to detach");
        bool online = true, doCleanup = false;
        const int eventListSz = 10;
        epoll_event eventList[eventListSz];
        while (online) {
            int eventNum = epoll_wait(epFd, eventList, eventListSz, -1);
            if (eventNum == -1)
                throw SysError(errno, "epoll_wait failed");
            else if (eventNum > 0) {
                for (int i = 0; i < eventNum; i++) {
                    epoll_event tmpEv = eventList[i];
                    if (tmpEv.data.fd == sockFd) {
                        auto tmpres = sockEvHandler(sockFd, tmpEv);
                        if (tmpres != 1) {
                            // container has stopped
                            loggerInstance()->info("connection closed");
                            doCleanup = true;
                            online = false;
                            break;
                        }
                    } else if (tmpEv.data.fd == STDIN_FILENO) {
                        // detach
                        auto tmpres = stdinEvHandler(sockFd, tmpEv);
                        if (tmpres == 0)
                            loggerInstance()->info("detaching");
                        if (tmpres == -1)
                            loggerInstance()->error("read input reach end of file");
                        if (tmpres != 1) {
                            online = false;
                            break;
                        }
                    } else {
                        loggerInstance()->error("unknow event, exiting");
                        online = false;
                        break;
                    }
                }
            }
        }
        clean();
        if (doCleanup)
            cleanup(containerId, true);
        return;
    } catch (const std::exception &e) {
        loggerInstance()->error("attach to container", container, "failed:", e.what());
    } catch (...) {
        loggerInstance()->error("attach to container", container, "failed");
    }
    clean();
}

#endif // ATTACH_CPP
