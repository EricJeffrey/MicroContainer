
# C++微容器工具

命令行微容器工具，使用C++17编写。

依赖:
 - [httplib.h](https://github.com/yhirose/cpp-httplib)作为http客户端
 - [Logger](https://github.com/EricJeffrey/linux_learn/blob/master/effective_io_model/reactor_model/Logger.h)作为日志工具。
 - [nlohmann/json](https://github.com/nlohmann/json)作为JSON处理工具
 - [libarchive](https://github.com/libarchive/libarchive/)提取镜像内容
 - [leveldb](https://github.com/google/leveldb)作为镜像数据库

链接选项: `-Ilib -D CPPHTTPLIB_OPENSSL_SUPPORT -lssl -lz -lcrypto -lpthread -larchive -lleveldb --std=c++17`
