#if !defined(CONTAINER_REPO_CPP)
#define CONTAINER_REPO_CPP

#include "container_repo.h"

#include <unordered_map>
#include <string>
#include <istream>
#include <fstream>
#include <stdexcept>
#include <exception>

#include "logger.h"
#include "json.hpp"

using std::string;
using std::unordered_map;

string toString(const ContState state) {
    if (state < 0 || state >= ContState::maxval)
        throw std::runtime_error("invalid container state");
    return contState2Desc[state];
}

ContState fromString(const string &state) {
    int res = 0;
    for (auto &&desc : contState2Desc) {
        if (desc == state)
            return ContState(res);
        ++res;
    }
    throw std::runtime_error("invalid container state desc");
}

void to_json(nlohmann::json &j, const ContInfoType &containerInfo) {
    j = {
        {"name", containerInfo.name},
        {"image", containerInfo.image},
        {"state", toString(containerInfo.state)},
        {"created", containerInfo.created},
    };
}

void from_json(const nlohmann::json &j, ContInfoType &containerInfo) {
    containerInfo.name = j.at("name");
    containerInfo.image = j.at("image");
    containerInfo.state = fromString(j.at("state"));
    containerInfo.created = j.at("created");
}

ContainerRepo ContainerRepo::fromFile(const string &filePath) {
    try {
        std::ifstream repoFStream(filePath);
        string repoJsonStr((std::istreambuf_iterator<char>(repoFStream)),
                           std::istreambuf_iterator<char>());
        nlohmann::json repoJson = nlohmann::json::parse(repoJsonStr);
        ContainerRepo resRepo;
        return ContainerRepo(repoJson.get<ContRepoContent>());
    } catch (const std::exception &e) {
        loggerInstance()->error("Read container repo:", filePath, "failed");
        throw;
    }
}

const nlohmann::json ContainerRepo::defaultSpecConfig =
    "{\"ociVersion\":\"1.0.0\",\"process\":{\"terminal\":true,\"user\":{\"uid\":0,\"gid\":0},"
    "\"args\":[\"sh\"],\"env\":[\"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/"
    "bin\",\"TERM=xterm\"],\"cwd\":\"/"
    "\",\"capabilities\":{\"bounding\":[\"CAP_AUDIT_WRITE\",\"CAP_CHOWN\",\"CAP_DAC_OVERRIDE\","
    "\"CAP_FOWNER\",\"CAP_FSETID\",\"CAP_KILL\",\"CAP_MKNOD\",\"CAP_NET_BIND_SERVICE\",\"CAP_NET_"
    "RAW\",\"CAP_SETFCAP\",\"CAP_SETGID\",\"CAP_SETPCAP\",\"CAP_SETUID\",\"CAP_SYS_CHROOT\"],"
    "\"effective\":[\"CAP_AUDIT_WRITE\",\"CAP_CHOWN\",\"CAP_DAC_OVERRIDE\",\"CAP_FOWNER\",\"CAP_"
    "FSETID\",\"CAP_KILL\",\"CAP_MKNOD\",\"CAP_NET_BIND_SERVICE\",\"CAP_NET_RAW\",\"CAP_SETFCAP\","
    "\"CAP_SETGID\",\"CAP_SETPCAP\",\"CAP_SETUID\",\"CAP_SYS_CHROOT\"],\"inheritable\":[\"CAP_"
    "AUDIT_WRITE\",\"CAP_CHOWN\",\"CAP_DAC_OVERRIDE\",\"CAP_FOWNER\",\"CAP_FSETID\",\"CAP_KILL\","
    "\"CAP_MKNOD\",\"CAP_NET_BIND_SERVICE\",\"CAP_NET_RAW\",\"CAP_SETFCAP\",\"CAP_SETGID\",\"CAP_"
    "SETPCAP\",\"CAP_SETUID\",\"CAP_SYS_CHROOT\"],\"permitted\":[\"CAP_AUDIT_WRITE\",\"CAP_CHOWN\","
    "\"CAP_DAC_OVERRIDE\",\"CAP_FOWNER\",\"CAP_FSETID\",\"CAP_KILL\",\"CAP_MKNOD\",\"CAP_NET_BIND_"
    "SERVICE\",\"CAP_NET_RAW\",\"CAP_SETFCAP\",\"CAP_SETGID\",\"CAP_SETPCAP\",\"CAP_SETUID\",\"CAP_"
    "SYS_CHROOT\"],\"ambient\":[\"CAP_AUDIT_WRITE\",\"CAP_CHOWN\",\"CAP_DAC_OVERRIDE\",\"CAP_"
    "FOWNER\",\"CAP_FSETID\",\"CAP_KILL\",\"CAP_MKNOD\",\"CAP_NET_BIND_SERVICE\",\"CAP_NET_RAW\","
    "\"CAP_SETFCAP\",\"CAP_SETGID\",\"CAP_SETPCAP\",\"CAP_SETUID\",\"CAP_SYS_CHROOT\"]},"
    "\"rlimits\":[{\"type\":\"RLIMIT_NOFILE\",\"hard\":1048576,\"soft\":1048576}]},\"root\":{"
    "\"path\":\"rootfs\"},\"hostname\":\"crun\",\"mounts\":[{\"destination\":\"/"
    "proc\",\"type\":\"proc\",\"source\":\"proc\"},{\"destination\":\"/"
    "dev\",\"type\":\"tmpfs\",\"source\":\"tmpfs\",\"options\":[\"nosuid\",\"strictatime\",\"mode="
    "755\",\"size=65536k\"]},{\"destination\":\"/dev/"
    "pts\",\"type\":\"devpts\",\"source\":\"devpts\",\"options\":[\"nosuid\",\"noexec\","
    "\"newinstance\",\"ptmxmode=0666\",\"mode=0620\",\"gid=5\"]},{\"destination\":\"/dev/"
    "shm\",\"type\":\"tmpfs\",\"source\":\"shm\",\"options\":[\"nosuid\",\"noexec\",\"nodev\","
    "\"mode=1777\",\"size=65536k\"]},{\"destination\":\"/dev/"
    "mqueue\",\"type\":\"mqueue\",\"source\":\"mqueue\",\"options\":[\"nosuid\",\"noexec\","
    "\"nodev\"]},{\"destination\":\"/"
    "sys\",\"type\":\"sysfs\",\"source\":\"sysfs\",\"options\":[\"nosuid\",\"noexec\",\"nodev\","
    "\"ro\"]},{\"destination\":\"/sys/fs/"
    "cgroup\",\"type\":\"cgroup\",\"source\":\"cgroup\",\"options\":[\"nosuid\",\"noexec\","
    "\"nodev\",\"relatime\",\"ro\"]}],\"linux\":{\"resources\":{},\"namespaces\":[{\"type\":"
    "\"pid\"},{\"type\":\"network\"},{\"type\":\"ipc\"},{\"type\":\"uts\"},{\"type\":\"mount\"}],"
    "\"seccomp\":{\"defaultAction\":\"SCMP_ACT_ERRNO\",\"architectures\":[\"SCMP_ARCH_X86_64\","
    "\"SCMP_ARCH_X86\",\"SCMP_ARCH_X32\"],\"syscalls\":[{\"names\":[\"accept\",\"accept4\","
    "\"access\",\"adjtimex\",\"alarm\",\"bind\",\"brk\",\"capget\",\"capset\",\"chdir\",\"chmod\","
    "\"chown\",\"chown32\",\"clock_adjtime\",\"clock_getres\",\"clock_gettime\",\"clock_"
    "nanosleep\",\"close\",\"connect\",\"copy_file_range\",\"creat\",\"dup\",\"dup2\",\"dup3\","
    "\"epoll_create\",\"epoll_create1\",\"epoll_ctl\",\"epoll_ctl_old\",\"epoll_pwait\",\"epoll_"
    "wait\",\"epoll_wait_old\",\"eventfd\",\"eventfd2\",\"execve\",\"execveat\",\"exit\",\"exit_"
    "group\",\"faccessat\",\"fadvise64\",\"fadvise64_64\",\"fallocate\",\"fanotify_mark\","
    "\"fchdir\",\"fchmod\",\"fchmodat\",\"fchown\",\"fchown32\",\"fchownat\",\"fcntl\",\"fcntl64\","
    "\"fdatasync\",\"fgetxattr\",\"flistxattr\",\"flock\",\"fork\",\"fremovexattr\",\"fsetxattr\","
    "\"fstat\",\"fstat64\",\"fstatat64\",\"fstatfs\",\"fstatfs64\",\"fsync\",\"ftruncate\","
    "\"ftruncate64\",\"futex\",\"futimesat\",\"getcpu\",\"getcwd\",\"getdents\",\"getdents64\","
    "\"getegid\",\"getegid32\",\"geteuid\",\"geteuid32\",\"getgid\",\"getgid32\",\"getgroups\","
    "\"getgroups32\",\"getitimer\",\"getpeername\",\"getpgid\",\"getpgrp\",\"getpid\",\"getppid\","
    "\"getpriority\",\"getrandom\",\"getresgid\",\"getresgid32\",\"getresuid\",\"getresuid32\","
    "\"getrlimit\",\"get_robust_list\",\"getrusage\",\"getsid\",\"getsockname\",\"getsockopt\","
    "\"get_thread_area\",\"gettid\",\"gettimeofday\",\"getuid\",\"getuid32\",\"getxattr\","
    "\"inotify_add_watch\",\"inotify_init\",\"inotify_init1\",\"inotify_rm_watch\",\"io_cancel\","
    "\"ioctl\",\"io_destroy\",\"io_getevents\",\"ioprio_get\",\"ioprio_set\",\"io_setup\",\"io_"
    "submit\",\"ipc\",\"kill\",\"lchown\",\"lchown32\",\"lgetxattr\",\"link\",\"linkat\","
    "\"listen\",\"listxattr\",\"llistxattr\",\"_llseek\",\"lremovexattr\",\"lseek\",\"lsetxattr\","
    "\"lstat\",\"lstat64\",\"madvise\",\"memfd_create\",\"mincore\",\"mkdir\",\"mkdirat\","
    "\"mknod\",\"mknodat\",\"mlock\",\"mlock2\",\"mlockall\",\"mmap\",\"mmap2\",\"mprotect\",\"mq_"
    "getsetattr\",\"mq_notify\",\"mq_open\",\"mq_timedreceive\",\"mq_timedsend\",\"mq_unlink\","
    "\"mremap\",\"msgctl\",\"msgget\",\"msgrcv\",\"msgsnd\",\"msync\",\"munlock\",\"munlockall\","
    "\"munmap\",\"nanosleep\",\"newfstatat\",\"_newselect\",\"open\",\"openat\",\"pause\",\"pipe\","
    "\"pipe2\",\"poll\",\"ppoll\",\"prctl\",\"pread64\",\"preadv\",\"preadv2\",\"prlimit64\","
    "\"pselect6\",\"pwrite64\",\"pwritev\",\"pwritev2\",\"read\",\"readahead\",\"readlink\","
    "\"readlinkat\",\"readv\",\"recv\",\"recvfrom\",\"recvmmsg\",\"recvmsg\",\"remap_file_pages\","
    "\"removexattr\",\"rename\",\"renameat\",\"renameat2\",\"restart_syscall\",\"rmdir\",\"rt_"
    "sigaction\",\"rt_sigpending\",\"rt_sigprocmask\",\"rt_sigqueueinfo\",\"rt_sigreturn\",\"rt_"
    "sigsuspend\",\"rt_sigtimedwait\",\"rt_tgsigqueueinfo\",\"sched_getaffinity\",\"sched_"
    "getattr\",\"sched_getparam\",\"sched_get_priority_max\",\"sched_get_priority_min\",\"sched_"
    "getscheduler\",\"sched_rr_get_interval\",\"sched_setaffinity\",\"sched_setattr\",\"sched_"
    "setparam\",\"sched_setscheduler\",\"sched_yield\",\"seccomp\",\"select\",\"semctl\","
    "\"semget\",\"semop\",\"semtimedop\",\"send\",\"sendfile\",\"sendfile64\",\"sendmmsg\","
    "\"sendmsg\",\"sendto\",\"setfsgid\",\"setfsgid32\",\"setfsuid\",\"setfsuid32\",\"setgid\","
    "\"setgid32\",\"setgroups\",\"setgroups32\",\"setitimer\",\"setpgid\",\"setpriority\","
    "\"setregid\",\"setregid32\",\"setresgid\",\"setresgid32\",\"setresuid\",\"setresuid32\","
    "\"setreuid\",\"setreuid32\",\"setrlimit\",\"set_robust_list\",\"setsid\",\"setsockopt\",\"set_"
    "thread_area\",\"set_tid_address\",\"setuid\",\"setuid32\",\"setxattr\",\"shmat\",\"shmctl\","
    "\"shmdt\",\"shmget\",\"shutdown\",\"sigaltstack\",\"signalfd\",\"signalfd4\",\"sigreturn\","
    "\"socket\",\"socketcall\",\"socketpair\",\"splice\",\"stat\",\"stat64\",\"statfs\","
    "\"statfs64\",\"statx\",\"symlink\",\"symlinkat\",\"sync\",\"sync_file_range\",\"syncfs\","
    "\"sysinfo\",\"syslog\",\"tee\",\"tgkill\",\"time\",\"timer_create\",\"timer_delete\","
    "\"timerfd_create\",\"timerfd_gettime\",\"timerfd_settime\",\"timer_getoverrun\",\"timer_"
    "gettime\",\"timer_settime\",\"times\",\"tkill\",\"truncate\",\"truncate64\",\"ugetrlimit\","
    "\"umask\",\"uname\",\"unlink\",\"unlinkat\",\"utime\",\"utimensat\",\"utimes\",\"vfork\","
    "\"vmsplice\",\"wait4\",\"waitid\",\"waitpid\",\"write\",\"writev\",\"mount\",\"umount2\","
    "\"reboot\",\"name_to_handle_at\",\"unshare\"],\"action\":\"SCMP_ACT_ALLOW\"},{\"names\":["
    "\"personality\"],\"action\":\"SCMP_ACT_ALLOW\",\"args\":[{\"index\":0,\"value\":0,\"op\":"
    "\"SCMP_CMP_EQ\"}]},{\"names\":[\"personality\"],\"action\":\"SCMP_ACT_ALLOW\",\"args\":[{"
    "\"index\":0,\"value\":8,\"op\":\"SCMP_CMP_EQ\"}]},{\"names\":[\"personality\"],\"action\":"
    "\"SCMP_ACT_ALLOW\",\"args\":[{\"index\":0,\"value\":131072,\"op\":\"SCMP_CMP_EQ\"}]},{"
    "\"names\":[\"personality\"],\"action\":\"SCMP_ACT_ALLOW\",\"args\":[{\"index\":0,\"value\":"
    "131080,\"op\":\"SCMP_CMP_EQ\"}]},{\"names\":[\"personality\"],\"action\":\"SCMP_ACT_ALLOW\","
    "\"args\":[{\"index\":0,\"value\":4294967295,\"op\":\"SCMP_CMP_EQ\"}]},{\"names\":[\"arch_"
    "prctl\"],\"action\":\"SCMP_ACT_ALLOW\"},{\"names\":[\"modify_ldt\"],\"action\":\"SCMP_ACT_"
    "ALLOW\"},{\"names\":[\"clone\"],\"action\":\"SCMP_ACT_ALLOW\",\"args\":[{\"index\":0,"
    "\"value\":2080505856,\"op\":\"SCMP_CMP_MASKED_EQ\"}]},{\"names\":[\"chroot\"],\"action\":"
    "\"SCMP_ACT_ALLOW\"}]},\"maskedPaths\":[\"/proc/kcore\",\"/proc/latency_stats\",\"/proc/"
    "timer_list\",\"/proc/timer_stats\",\"/proc/sched_debug\",\"/sys/"
    "firmware\"],\"readonlyPaths\":[\"/proc/asound\",\"/proc/bus\",\"/proc/fs\",\"/proc/irq\",\"/"
    "proc/sys\",\"/proc/sysrq-trigger\"]}}"_json;

#endif // CONTAINER_REPO_CPP
