
# C++微容器工具

命令行微容器工具，使用C++17编写。

依赖:
 - [httplib.h](https://github.com/yhirose/cpp-httplib) - http客户端
 - [Logger](https://github.com/EricJeffrey/MicroContainer/blob/master/lib/logger.h) - 日志工具。
 - [nlohmann/json](https://github.com/nlohmann/json) - JSON处理工具
 - [libarchive](https://github.com/libarchive/libarchive/) - 提取镜像内容
 - [leveldb](https://github.com/google/leveldb) - 镜像数据库
 - [CLI11](https://github.com/CLIUtils/CLI11) - 命令行解析工具

链接选项: `-Ilib -D CPPHTTPLIB_OPENSSL_SUPPORT -lssl -lz -lcrypto -lpthread -larchive -lleveldb --std=c++17`
