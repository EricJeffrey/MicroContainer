# 使用varlink协议访问podman服务，创建容器，获取容器信息，结束容器


import json
import varlink

addr = "unix:/run/podman/io.podman"
iface = "io.podman"
method = "ListContainers"
client = varlink.Client("unix:/run/podman/io.podman")
connection = client.open("io.podman")
containerlist = connection._call("ListContainers")['containers']
print(containerlist[0]['names'])

