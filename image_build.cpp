#if !defined(IMAGE_BUILD_CPP)
#define IMAGE_BUILD_CPP

#include "lib/logger.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <istream>
#include <string>

using std::string;

void parseLine(const string &line) {}

void build(const string &filePath, const string &nameTag) noexcept {
    try {
        if (!std::filesystem::exists(filePath)) {
            loggerInstance()->info("please specify path of file to use");
            return;
        }
        string imageName = nameTag;
        if (imageName.empty())
            imageName = genRandomStr(8) + ":latest";
        std::ifstream fileStream{filePath};
        string line;
        while (std::getline(fileStream, line)) {
            trim(line);
            if (!line.empty()) {
                // parse here
            }
        }
    } catch (const std::exception &e) {
        loggerInstance()->error("build failed:", e.what());
    } catch (...) {
        loggerInstance()->error("build failed:");
    }
}

#endif // IMAGE_BUILD_CPP
