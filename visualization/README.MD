# 容器可视化

python+echarts实现

## 后端python

- 使用multiprocessing包实现多进程
  - 服务器进程 + podman-stats进程
  - 使用管道`os.pipe`+`os.dup2`将标准输出流`stdout`复制到`管道`
  - 使用其中的`Queue`实现队列存放数据
- 使用`wsgiref.simple_server`的`make_server`实现简易服务器
- 使用`podman-stats`获取容器运行状态

## 前端echarts

使用echarts+jQuery实现异步加载数据，使用setinterval实现定时任务