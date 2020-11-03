#if !defined(UTILS_CPP)
#define UTILS_CPP

#include "utils.h"
#include "lib/logger.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>

#include <fcntl.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

vector<string> split(const string &str, const string &delimiter) {
    vector<string> res;
    size_t laPos = 0, curPos, sizeDeli = delimiter.size();
    if (sizeDeli == 0)
        return {};
    curPos = str.find_first_of(delimiter);
    while (curPos != string::npos) {
        res.emplace_back(str.substr(laPos, curPos - laPos));
        laPos = curPos + sizeDeli;
        curPos = str.find_first_of(delimiter, laPos);
    }
    if (laPos < sizeDeli)
        res.emplace_back(str.substr(laPos));
    return res;
}

int fork_exec_wait(const string &filePath, const vector<string> &args, bool noStdIO) {
    pid_t child = fork();
    if (child == -1)
        throw SysError(errno, "Call to fork failed");
    else if (child == 0) { // child
        if (noStdIO)
            close(STDOUT_FILENO), close(STDERR_FILENO), close(STDIN_FILENO);
        const size_t sz = args.size();
        char *argv[sz + 1];
        for (size_t i = 0; i < sz; i++)
            argv[i] = strdup(args[i].c_str());
        argv[sz] = nullptr;
        if (execv(filePath.c_str(), argv) == -1)
            exit(1);
    } else { // parent
        int statLoc = -1;
        if (waitpid(child, &statLoc, 0) == -1)
            throw SysError(errno, "Call to waitpid failed");
        else if (WIFEXITED(statLoc))
            return WEXITSTATUS(statLoc);
        else
            throw IncorrectlyExitError("Child returned incorrectly");
    }
    return 0;
}

string genRandomStr(int len) {
    string res(size_t(len), '0');
    static constexpr char alphanum[] = "0123456789"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    static std::uniform_int_distribution<int> distribution(0, 61);
    for (int i = 0; i < len; ++i)
        res[i] = alphanum[distribution(generator)];
    return res;
}

string concat(const vector<string> &argv) {
    string res;
    for (auto &&x : argv)
        res.append(x).append(" ");
    return res;
};

// create file with content
void createFile(const string &filePath, const string &content) {
    int fd = open(filePath.c_str(), O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH);
    if (fd == -1) {
        loggerInstance()->sysError(errno, "Call to open ", filePath, "failed");
        throw runtime_error("call to open failed");
    }
    if (write(fd, content.c_str(), content.size()) == -1) {
        loggerInstance()->sysError(errno, "Call to write failed");
        throw runtime_error("call to write failed");
    }
    if (close(fd) == -1) {
        loggerInstance()->sysError(errno, "Call to close return -1");
    }
}

string sha256_string(const char *str, size_t len) {
    char output_buffer[65];
    std::fill(output_buffer, output_buffer + 65, 0);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, len);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_buffer + (i * 2), "%02x", hash[i]);
    }
    return string(output_buffer);
}

string nowISO() {
    time_t now;
    time(&now);
    char buf[30] = {};
    strftime(buf, sizeof(buf), "%F %T%Z", gmtime(&now));
    return buf;
}

void lineupPrint(std::ostream &out, const vector<vector<string>> &lines, bool left, int spacing) {
    vector<int> widths;
    for (size_t i = 0; i < lines.size(); i++)
        for (size_t j = 0; j < lines[i].size(); j++)
            if (j >= widths.size())
                widths.push_back(lines[i][j].size());
            else
                widths[j] = std::max(widths[j], (int)lines[i][j].size());
    for (size_t i = 0; i < lines.size(); i++) {
        for (size_t j = 0; j < lines[i].size(); j++)
            out << (left ? std::left : std::right) << std::setw(widths[j] + spacing) << lines[i][j];
        out << std::endl;
    }
}

#endif // UTILS_CPP
