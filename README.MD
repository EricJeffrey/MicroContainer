
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

# 设计
- [微容器工具设计文档](#微容器工具设计文档)
  - [功能需求](#功能需求)
  - [详细功能描述](#详细功能描述)
    - [pull](#pull)
    - [build](#build)
    - [image ls](#image-ls)
    - [image rm](#image-rm)
    - [create](#create)
    - [container ls](#container-ls)
    - [container rm](#container-rm)
    - [start](#start)
    - [run](#run)
    - [attach](#attach)
    - [stop](#stop)
    - [cleanup](#cleanup)
  - [容器/镜像相关的数据结构](#容器镜像相关的数据结构)
    - [镜像、layer、容器之间的关系:](#镜像layer容器之间的关系)
    - [数据结构概述](#数据结构概述)

## 功能需求

支持的功能

- 镜像
  - `pull`          从registry拉取镜像并保存
  - `build`         使用Dockerfile创建镜像并保存
  - `image ls`      列出镜像
  - `image rm`      删除某个镜像
- 容器
  - `create`        创建容器并保存
  - `container ls`  列出所有容器
  - `container rm`  删除某个容器
  - `start`         启动容器
  - `run`           运行容器，使用某个镜像创建容器，启动并在其中执行某个命令
  - `attach`        附加到某个正在运行的容器
  - `stop`          停止一个正在运行的容器
  - `cleanup`       清理已退出容器的挂载点、netns配置等

功能简述 - 镜像:
```
镜像数据=manifest+config

pull: 根据分发格式从registry获取镜像信息，将[镜像数据]保存，更新本地镜像仓库
build: 根据Dockerfile的内容创建所需的[镜像数据]，保存并更新仓库
ls: 获取本地仓库数据并打印
rm: 删除镜像的[镜像数据]，更新仓库
```

功能简述-容器:
```
容器数据=OCIConfig+lower+dirs
配置NS=配置容器的网络Namespace、网卡、路由表等并写入到容器的OCIConfig

create: 根据[镜像数据]创建容器数据，更新本地容器仓库
ls: 获取本地仓库数据并打印
rm: 删除[容器数据]，更新本地容器仓库
start: 配置NS，挂载文件夹，conmon启动容器
run: 根据参数修改容器OCIConfig，创建容器，配置NS，conmon启动容器
attach: 创建伪终端，将输入输出转发到容器的attach套接字
stop: 发送term信号，超时后发送kill
cleanup: 清理已退出容器的挂载点、netns配置、iptable配置、虚拟网卡
```

----- 

## 详细功能描述

### pull

`pull name:tag`，若本地已存在该镜像则退出，否则从默认`registry/library`地址拉取镜像并保存到本地。主要包括**获取镜像数据**和**保存镜像**两个步骤。

镜像数据获取需要参考[镜像分发规范](https://github.com/opencontainers/distribution-spec/blob/master/spec.md)以及其它相关文档，主要步骤为: *获取镜像清单文件manifest* --> *获取镜像配置文件config.json* --> *获取镜像每层的数据blob* --> *解压blob*

保存镜像: 保存镜像的 **清单文件manifest** 和 **配置文件config.json**，更新 **镜像数据库** 和 **layer数据库**。

### build 

`build [-f file] [-t name]`， 使用指定的 *Dockerfile* 创建镜像。支持的语法:
- FROM \<image>:\<tag>
- CMD ["executable","param1","param2"]
- COPY \<src> \<dest> 可指定多个
- ENV \<key>=\<value> 可指定多个

`CMD`与`ENV`可以通过修改镜像的配置文件得到，`COPY`可以通过在镜像目录中创建新的`diff`目录实现。

步骤: 
- 获取文件，生成镜像名称、ID，创建镜像目录、diff目录
- 读取基础镜像`FROM`，加载基础镜像配置文件中`container_config`的内容作为配置信息
- 读取文件行并对每一行解析: 
  - ENV 添加到配置信息中
  - COPY 拷贝文件到镜像的diff目录中
  - CMD 修改配置信息中CMD的内容，清空Entrypoint
- 获得配置内容的sha256作为digest
- 生成清单文件
  - config - digest与size
  - layers - 从基础镜像清单文件获得
  - mediaType: application/vnd.docker.distribution.manifest.v2+json
  - schemaVersion: 2
- 更新镜像数据库

### image ls

`image ls`， 读取镜像数据库的内容，格式化并打印。

### image rm

`image rm name:tag/ID`，根据镜像名获取镜像ID，读取容器数据库判断镜像是否被使用，未被使用 - 删除清单、配置文件、只有当前镜像引用的层，更新镜像、layer数据库，否则提示并退出。

### create

`create imgId name`，使用镜像创建容器。获取镜像配置文件，生成容器OCI配置文件，生成lower目录文件，创建待挂载目录`merged, upper, lower, diff`，若镜像目录下有`diff`目录，则拷贝到对应容器目录，更新容器数据库，更新镜像数据库。生成容器OCI配置文件时需要:

- 更新OCI配置netns的文件路径path
- 更新命令、环境、hostname

### container ls

`container ls`，读取容器数据库内容，格式化并打印

### container rm

`container rm name/ID`，删除容器目录、OCI配置文件、lower文件，更新容器、镜像数据库。


### start

`start name/ID`，启动容器，表现为容器内进程执行，具体步骤:

- 检查容器是否存在以及容器是否未运行
- 创建netns
- 配置netns内的路由表、网关、端口映射
- 获取可用IP
- 读取lower文件，挂载目录
- 调用`conmon`启动容器
  - 创建 `container-pidfile`, `conmon-pidfile`, `conmon-logfile`, `oci-logfile`
  - 配置容器退出后的清理程序 - [cleanup](#cleanup)
  - 记录`conmon`和容器进程的PID
- 更新容器数据库

与conmon同步：
- conmon会通过`crun create`创建容器，但不会启动容器，因此需要在创建之后调用`crun start`启动容器
- conmon通过从环境变量获得名为`_OCI_SYNCPIPE`的管道文件描述符，向父进程通知容器创建完成，格式为`{"data": pid}`

### run

`run [-na] image [args]`，创建容器并在其中执行某个程序，可以指定创建后的容器名、要执行的程序。该命令实际由 **创建** 和 **启动** 两个步骤组成，在创建容器时需要根据 `args` 参数更改容器配置文件的执行程序。

参数:
- -n 指定名字
- -a 附加到启动的容器，创建、启动然后附加
- args 容器内的程序，若不指定则为镜像默认的命令

### attach

`attach name/ID`，附加到容器。获取容器unix套接字，重定向IO到套接字。`conmon`自动创建了名为`attach`的unix套接字，类型为`SOCK_SEQPACKET`，可借此实现。

- 获取套接字
- 连接到套接字
- 设置终端为raw模式
- 使用epoll监听套接字和标准输入的事件
  - 将标准输入转发到套接字
  - 将套接字的返回内容转发到终端
    - 根据响应内容第一个字符判断stdout和stderr，2表示stdout，3表示stderr
    - 由于conmon在附加并终止容器进程(attach容器并`exit`)时不会调用`cleanup`，故若套接字断开，则**停止并清理**容器

### stop

`stop name/ID`，停止容器。获取容器进程PID，发送`SIGTERM`信号，超时后判断`/proc/PID`是否存在(是否已经结束)，若存在，则发送`SIGKILL`信号。容器进程结束后conmon会自动调用 [cleanup](#cleanup) 更新容器数据库。

### cleanup

`cleanup name/ID`，清理容器的相关文件，包括conmon创建的文件(套接字和FIFO-attach, winsz, shm, ctl)，清理挂载点、删除网络命名空间、删除虚拟网卡，删除运行时的容器，更新容器数据库。

----- 

## 容器/镜像相关的数据结构

使用[LevelDB](https://github.com/google/leveldb/)作为数据库引擎。

### 镜像、layer、容器之间的关系:

```
容器 - 配置文件+目录+程序
  配置文件 - 来自于镜像
  目录 - 由 upper+lower 挂载组合得到
    upper - 容器创建的目录
    lower - 来自于镜像所包含的每一层layer
  程序 - 镜像配置文件或用户指定，只在容器启动后表现为进程

镜像 - 清单+配置+layers
  清单 - 包含镜像的哈希、镜像每一层的哈希
  配置 - 包含创建容器的配置
  layers - 组成镜像的文件(夹)，会在创建容器时挂载组成容器目录

layer - 文件系统(部分)的压缩包

关联：
  镜像 - 镜像数据库、layer数据库
  容器 - 容器数据库、镜像数据库
```

### 数据结构概述

- layer
  - layer仓库项 - 引用数+引用该layer的镜像ID
  - layer仓库 - 所有layer信息
- 镜像相关
  - 镜像仓库项 - 镜像仓库每个镜像的信息
  - 镜像仓库 - 存储本地的镜像信息
  - 镜像清单Manifest/配置 - 暂不使用，后续build操作中可能使用
- 容器相关
  - 容器仓库项 - 容器仓库中每个容器的信息
  - 容器仓库 - 本地容器信息
  - 容器配置信息 - 容器的配置信息
  - 容器OCI规范信息 - 容器的OCI-SPEC信息

伪代码
```
LayerRepoItem{
    id, usedImgCnt
}
ContainerRepoItem{
    id, ImageID, name, created, status{contStatus, time}
}
ImageRepoItem{
    regAddr, id, name, tag, created, usedContCnt
}

LayerRepo{
    add(LayerID)
    update(LayerID, increase)
    remove(LayerID)
    foreach()
}

ImageRepo {
    add(ImageRepoItem)
    contains(nametag/id)
    updateUseCnt(ImageID, increase)
    remove(nametag/id)
    getItem(nametag/id)
    print(ostream)
    foreach()
}

ContainerRepo{
    contains(name/id)
    add(ContainerRepoItem)
    updateStatus(containerID, status)
    remove(name/id)
    getItem()
    print(ostream)
    isImageUsed(ImageID)
    foreach()
}
```
