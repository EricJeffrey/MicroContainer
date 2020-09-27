
# C++微容器工具

依赖:
 - [httplib.h](https://github.com/yhirose/cpp-httplib)作为http客户端
 - [Logger](https://github.com/EricJeffrey/linux_learn/blob/master/effective_io_model/reactor_model/Logger.h)作为日志工具。
 - [nlohmann/json](https://github.com/nlohmann/json)作为JSON处理工具
 - [libarchive](https://github.com/libarchive/libarchive/)提取镜像内容

编译选项: `-D CPPHTTPLIB_OPENSSL_SUPPORT -lssl -lz -lcrypto -lpthread -larchive`

## 功能分析与实现

OCI给出了镜像的[分发规范](https://github.com/opencontainers/distribution-spec/blob/master/spec.md)，该规范基于[Docker Registry HTTP API V2 protocol](https://github.com/docker/distribution/blob/5cb406d511b7b9163bff9b6439072e4892e5ae3b/docs/spec/api.md)，详细定义了镜像相关动作、文件等。下表定义了registry具备的功能以及对应的接口，其中`1a`接口可以用来验证registry是否支持v2:

| ID | Method | API endpoint | Accepted Successful Response Codes | Accepted Failure Response Codes |
| ---| --- | ---|---|---|
| 1a | `GET` | `/v2/` | `200` | `404`/`401` |
| 2a | `GET` | `/v2/<name>/blobs/<digest>` | `200` | `404` |
| 3a | `GET` | `/v2/<name>/manifests/<reference>` | `200` | `404` |
| 4a | `POST` | `/v2/<name>/blobs/uploads/` | `202` | `404` |
| 4b | `POST` | `/v2/<name>/blobs/uploads/?digest=<digest>` | `201`/`202` | `404`/`400` |
| 5a | `PATCH` | `/v2/<name>/blobs/uploads/<reference>` | `202` | `404`/`416` |
| 6a | `PUT` | `/v2/<name>/blobs/uploads/<reference>?digest=<digest>` | `201` | `404`/`400` |
| 7a | `PUT` | `/v2/<name>/manifests/<reference>` | `201` | `404` |
| 8a | `GET` | `/v2/<name>/tags/list` | `200`  | `404` |
| 8b | `GET` | `/v2/<name>/tags/list?n=<integer>&last=<integer>` | `200` | `404` |
| 9a | `DELETE` | `/v2/<name>/manifests/<reference>` | `202` | `404`/`400`/`405` |
| 10a | `DELETE` | `/v2/<name>/blobs/<digest>` | `202` | `404`/`405` |



### Pull镜像
镜像是分层的，因此获取镜像一般时获取每一层的文件系统，然后通过`mount`挂载到一个地方成为镜像。Pull的过程主要包含两步: 获取`manifest` 和 获取`blob`。

- manifest: `3a GET /v2/library/python/manifests/latest`，镜像的清单文件，包括镜像每一层的哈希，镜像的签名等，具体信息可以参考[OCI-image-spec](https://github.com/opencontainers/image-spec/blob/master/manifest.md#image-manifest) [docker-image-manifest-v2-1](https://github.com/docker/distribution/blob/5cb406d511b7b9163bff9b6439072e4892e5ae3b/docs/spec/manifest-v2-1.md) [docker-image-manifest-v2-2](https://github.com/docker/distribution/blob/5cb406d511b7b9163bff9b6439072e4892e5ae3b/docs/spec/manifest-v2-2.md)。获取时可以指定`Accept`首部，表面需要的manifest版本类型。
- blobs: `2a GET /v2/library/python/blobs/sha256:xxxx`，镜像每一层的内容，一般为`.tar.gz`文件，manifest中会指出每一层的哈希

对于`v2-schema1`的Manifest，其中的`history[0]["v1Compatibility]`即是镜像的配置文件。

对于`v2-schema2`的Manifest，可以使用其中的`config.digest`作为ref，通过`/v2/name/blobs/ref`获得镜像的配置文件，但若返回404，则仍需通过上述`schema1`的manifest获得。

实现上，忽略manifest版本兼容性，假设`pull name:tag`，步骤如下

1. `GET registry/v2/` 检查registry是否可以访问
2. `GET registry/v2/library/name/manifests/tag` 获取清单文件
   1. 对于`schema1`的清单文件
      1. `fsLayers`中的`blobSum`为镜像压缩包，通过`/v2/name/blobs/blobSum`获取
      2. `history[0][v1Compatibility]`为镜像配置文件
      3. 哈希可能需要通过获得的算配置文件手动计算
   2. 对于`schema2`的清单文件
      1. `Layers`为每一层的哈希，通过此哈希获得该层的压缩包
      2. `config`中包含镜像配置信息的哈希，通过`/v2/name/blobs/ref`获得镜像配置
         1. 若无法获得配置则使用`schema1`清单文件中的信息
3. 获取并提取每一层的压缩包
4. 保存配置文件和清单

### 容器生命周期

文件: 

- 容器仓库文件`continerRepo.json`
  ```
  容器的状态 created/running/stopped
  创建时间 yyyy-MM-dd'T'HH:mm:ss.SSSXXX
  使用的镜像ID imageID
  ```
- 容器的OCI配置文件`config.json`

生命周期:

1. 创建容器
2. 启动容器
   1. 附加到容器
   2. 在容器内执行程序
3. 停止容器

### 创建容器

`microc create iamge containername`

1. 检查容器仓库文件是否存在 - utils
   - 不存在则创建仓库文件 - {}
2. 生成容器名哈希
3. 验证容器名是否被占用
   - 读取仓库文件并检验哈希值
4. 验证镜像是否存在
   - 读取镜像仓库遍历查找: name+tag/id
5. 读取镜像的 manifest[layers] 中的每一个 digest
6. 创建容器目录，merged work diff
7. 保存digest到容器目录lower文件内
8. 使用容器id创建netns
9.  读取镜像config，获取其中 {Cmd, Entrypoint, Env} 的值
10. 创建config.json
   - 修改root[path]=merged, hostname=shaId[0:12], cmd=Cmd/Entrypoint, env=Env, netns.pat
12. 保存容器配置信息，更新容器仓库信息


### 启动容器

容器的输出需要后台daemon程序使用命名管道方式记录。

1. 挂载容器文件目录
   1. `mount overlay...`
2. 配置网桥、虚拟网卡，网络命名空间netns - `fork&exec() + ip link + ip addr + iptables + route`
   1. 配置设备地址
   2. 配置端口映射，netns内的网关
   3. 使用文件保持netns
3. 生成`config.json`，设置步骤2创建的netns
4. 根据参数配置`{terminal: true/false}`
   1. -i - 交互式运行，保持标准IO打开
   2. -t - 分配伪终端，将容器标准IO附加到伪终端
   3. -d - 后台运行，重定向标准输入输出，不分配伪终端
   4. 执行`crun run ...`
5. 配置pidfile
6. 记录容器状态 - 启动时间...
