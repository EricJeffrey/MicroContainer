# 微容器项目

# 需求分析

面向物联网节点的微容器设计：针对现有容器技术系统资源需求高的问题，提出适用于不同量级节点的微容器技术；提供受限资源优化利用机制，提高边缘节点的智能决策能力，降低请求时延，提高系统处理能力。

整体任务: 微容器设计，适用于不同量级的微容器技术，受限资源优化利用机制。

科研任务: 受限资源优化机制研究

工程任务: 设计适用于不同量级的微容器。

## 工程任务需求

由于Docker对设备的内存资源要求较大，资源受限的设备上不适合使用，因此需要设计能够适用于不同量级设备的微容器。

目标: 以runc为基础创建容器，开发轻量级的镜像打包工具，能够使用网络指令创建容器、获取容器信息及运行状态。创建容器资源可视化平台，能够查看容器信息，停止或启动容器，结束容器内进程等。

开源容器项目比较: 
1. `runc`，Go语言开发，Apache证书，可以闭源商用
2. `crun`，C语言开发，GNUGPLv2，产品可以商用，但必须使用相同证书开源

cRun更加轻量，性能更好。

## 详细功能

微容器工具
1. 能够使用给定的配置文件与根文件系统创建容器并运行
2. 能够采用用户友好方式(配置项询问、GUI选择、网页选择)创建容器配置，打包容器文件系统与配置文件

微容器辅助工具
1. 能够使用网络指令创建容器，采用TCP协议实现网络通信
2. 能够使用网络指令获取容器信息与运行状态，采用TCP方式获取信息

容器可视化平台
1. 能够可视化容器运行状态，内存使用情况的平台
