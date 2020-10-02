# FIFO+crun启动容器

attach的实现

## 一种可行方案

1. 后台进程
   1. 使用管道重定向容器的输入输出
   2. 使用unix套接字监听其他程序的attach
   3. 使用IO复用(`EPOLL`)监听套接字和管道
      1. 将容器输出转发到日志文件和其它程序
      2. 将attach的输入转发给管道(使用伪终端)
   4. 容器参数: `-d {terminal: false}`
2. attach进程
   1. 打开套接字
   2. 从标准输入读，转发到套接字
   3. 获取套接字的内容，转发到标准输出

## what do we need

1. 对于`{terminal: true}` 并且以`-d`启动时，应该如何处理终端相关设置，需要分配伪终端？或者重定向标准输入？
   1. `-d`启动时要求`{terminal:false}`
   2. 可以在attach时分配伪终端，然后连接到套接字并将输入发送到套接字即可
2. 需要将stdout的内容分别输出到日志和附加进程(daemon程序的管道)
   1. 需要类似conmon的后台程序管理容器进程的输入输出
-------
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

10-02,10:33 epoll在管道、套接字上的ERR、HUP事件如何处理？

## 容器启动进程

以有终端、detach方式启动容器，重定向标准输入输出到管道，创建unix套接字以提供客户端attach连接，在容器的写管道和attach套接字上poll。

- 用于读的管道 rpipe
- 用于写的管道 wpipe
- fork
  - 关闭不需要的端
  - stdin重定向到rpipe
  - stdout/stderr重定向到wpipe
  - execl启动容器
- 创建unix套接字，绑定
- 打开日志文件
- poll用于写的管道和套接字
  - 套接字连接 - 客户端attach，添加到poll池和输出池
  - 管道 - 读取输出，写入到日志文件，转发到输出池的每个套接字

## 另一个程序
- 打开unix套接字
- 连接服务端
- poll套接字和stdin
  - 将stdin的内容转发到套接字
  - 将套接字的内容输出到stdout

↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑

-------

podman - 若不读取stdin则可以后台运行，否则必须指定-t选项分配终端。
因此，将容器stdio重定向到daemon进程的管道，由daemon程序将容器进程的输入/输出转发到其它地方(poll/epoll)。也可以使用unix_socket的SOCK_STREAM服务端允许多个attach。
另外，终端内容是否可以直接读取/dev/pts而不通过daemon？