# podman 的 varlink API

podman提供了[varlink](https://varlink.org/)协议的API - [libpod/API](https://github.com/containers/libpod/blob/master/API.md). 

- `apt install libvarlink` 安装varlink
- varlink[地址格式](https://varlink.org/)可以为`TCP`、`unix socket`等
  - Podman的varlink地址格式: `unix:///run/podman/io.podman/`
  - 该地址包含`org.varlink.service io.podman`两个接口，如下的`interfaces`
    ```
    eric@ubuntu-server:~$ sudo varlink call unix:///run/podman/io.podman/    org.varlink.service.GetInfo
    {
      "interfaces": [
        "org.varlink.service",
        "io.podman"
      ],
      "product": "podman",
      "url": "https://github.com/containers/libpod",
      "vendor": "Atomic",
      "version": "1.6.2"
    }
    ```
- 调用方法: `sudo varlink call [ADDRESS/]INTERFACE.METHOD [ARGUMENTS]`
- 参数`ARGUMENTS`为JSON格式，例如`'{"name": "halo"}'`

-----

**使用varlink获取某个容器的状态**
```
eric@ubuntu-server:~$ sudo varlink call unix:///run/podman/io.podman/io.podman.GetContainerStats '{"name": "halo"}'
{
  "container": {
    "block_input": 0,
    "block_output": 0,
    "cpu": 3.677181975432204563741e-06,
    "cpu_nano": 456196608,
    "id": "d02ab31008010f469cd1aa7e170efd7a859311942e3a3171d883b42f9d57a2d9",
    "mem_limit": 2065936384,
    "mem_perc": 5.585085818402432122909e-01,
    "mem_usage": 11538432,
    "name": "halo",
    "net_input": 1258,
    "net_output": 2522,
    "pids": 1,
    "system_nano": 0
  }
}
```