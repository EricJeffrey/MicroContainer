#include <arpa/inet.h>
#include <pty.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

const char sockPath[] = "/run/user/1000/libpod/tmp/socket/"
                        "927b7a448cdfbd8fb42208d35c23c058b417e913f5d5fed215da705e58f54ff5/attach";
int stdinDataFd = 0, contDataFd = 0;
termios ttyOrigin;

void errExit(const char *msg, const char *desc = nullptr) {
    const int bufsz = strlen(msg) + (desc == nullptr ? 0 : strlen(desc)) + 25;
    char buf[bufsz] = {};
    snprintf(buf, bufsz - 1, "call to %s failed: %s", msg, desc);
    perror(buf);
    exit(1);
}

void ttyReset() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttyOrigin) == -1)
        errExit("tcsetattr");
    if (stdinDataFd != 0)
        close(stdinDataFd);
    if (contDataFd != 0)
        close(contDataFd);
}

int connect2UnSock(const char *sockPath) {
    int sockFd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sockFd == -1)
        errExit("socket");
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockPath, sizeof(addr.sun_path) - 1);
    if (connect(sockFd, (sockaddr *)&addr, sizeof(addr)) == -1)
        errExit("connect");
    return sockFd;
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
    t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
    /* Noncanonical mode, disable signals, extended
       input processing, and echoing */

    t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK | ISTRIP | IXON | PARMRK);
    /* Disable special handling of CR, NL, and BREAK.
       No 8th-bit stripping or parity error handling.
       Disable START/STOP output flow control. */

    // t.c_oflag &= ~OPOST; /* Disable all output processing */

    t.c_oflag |= OPOST;

    t.c_cc[VMIN] = 1;  /* Character-at-a-time input */
    t.c_cc[VTIME] = 0; /* with blocking */
    if (tcsetattr(fd, TCSAFLUSH, &t) == -1)
        return -1;
    return 0;
}

/*
获取当前终端属性
设置终端raw
设置atexit
创建伪终端pair - master slave
打开unix套接字
poll
    stdin数据转发到master
    slave转发到unix套接字
    unix套接字数据转发到slave
    master数据写到stdout
*/

// ctrl+p is 0x10, ctrl+q is 0x11
// FIXME 控制字符无法渲染，尚未找到解决方法
// g++ -g -Wall -lutil --std=c++17 -o attach_pty.out attach_pty.cpp
int main(int argc, char const *argv[]) {
    winsize ws;
    { // set current tty
        if (tcgetattr(STDIN_FILENO, &ttyOrigin) == -1)
            errExit("tcgetattr");
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)
            errExit("ioctl-TIOCGWINSZ");
        // reset terminal at exit
        if (atexit(ttyReset) != 0)
            errExit("atexit");
        if (ttySetRaw(STDIN_FILENO, &ttyOrigin) == -1)
            errExit("ttySetRaw");
    }
    // connect to sock
    int sockFd = connect2UnSock(sockPath);
    // epoll
    int epFd = epoll_create1(0);
    if (epFd == -1)
        errExit("epoll_create-0");
    { // add fds to epoll
        epoll_event tmpEvent;
        auto add2epoll = [&tmpEvent](int epFd, int fd, uint32_t events) {
            memset(&tmpEvent, 0, sizeof(tmpEvent));
            tmpEvent.data.fd = fd, tmpEvent.events = events;
            if (epoll_ctl(epFd, EPOLL_CTL_ADD, fd, &tmpEvent) == -1)
                errExit("epoll_ctl");
        };
        add2epoll(epFd, STDIN_FILENO, EPOLLIN | EPOLLHUP);
        add2epoll(epFd, sockFd, EPOLLIN | EPOLLRDHUP | EPOLLHUP);
    }
    auto stdinEvHandler = [&sockFd](epoll_event ev) {
        if (ev.events & EPOLLERR)
            errExit("EPOLLERR on STDIN_FILENO");
        char buf[BUFSIZ];
        size_t numRead = read(STDIN_FILENO, buf, BUFSIZ);
        if (numRead < 0)
            errExit("read-stdin");
        else if (numRead == 0)
            errExit("read EOF on stdin");
        else {
            if (buf[0] == '\x10') {
                fprintf(stderr, "\nbye\n");
                exit(0);
            }
            write(sockFd, buf, numRead);
        }
    };
    auto sockEvHandler = [&sockFd](epoll_event ev) {
        if (ev.events & (EPOLLERR)) {
            errExit("EPOLLERR on sock");
        } else if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
            fprintf(stdout, "sock conn closed");
            exit(1);
        } else {
            char buf[BUFSIZ];
            size_t numRead = read(sockFd, buf, BUFSIZ);
            if (numRead < 0)
                errExit("read-sock");
            else if (numRead == 0)
                errExit("read EOF on sock");
            else
                write(STDOUT_FILENO, buf, numRead);
        }
    };
    epoll_event eventList[100];
    while (true) {
        int ret = epoll_wait(epFd, eventList, 100, -1);
        if (ret == -1)
            errExit("epoll_wait");
        else if (ret > 0) {
            for (int i = 0; i < ret; i++) {
                epoll_event tmpEv = eventList[i];
                if (tmpEv.data.fd == sockFd)
                    sockEvHandler(tmpEv);
                else if (tmpEv.data.fd == STDIN_FILENO)
                    stdinEvHandler(tmpEv);
                else
                    errExit("unknow event fd");
            }
        }
    }
    return 0;
}

/*
stdinDataFd = open("./foo.txt", O_CREAT | O_TRUNC | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IWGRP | S_IRWXG | S_IROTH);
if (stdinDataFd == -1)
    errExit("open-foo.txt");
contDataFd = open("./bar.txt", O_CREAT | O_TRUNC | O_RDWR,
                  S_IRUSR | S_IWUSR | S_IWGRP | S_IRWXG | S_IROTH);
if (stdinDataFd == -1)
    errExit("open-bar.txt");
*/