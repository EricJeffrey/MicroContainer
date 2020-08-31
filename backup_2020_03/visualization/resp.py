from datetime import datetime
import time
import sys
import os
import json
import threading
import multiprocessing
import socketserver
from wsgiref.simple_server import make_server

stateQueue = multiprocessing.Queue(2500)


def statsProcess(inFd):
    """ 将 inFd 重定向到 stdout，执行 runc events """
    stdoutFd = sys.stdout.fileno()
    # print("statsProcess: infd=%d, stdoutfd=%d" % (inFd, stdoutFd))
    os.dup2(fd=inFd, fd2=stdoutFd, inheritable=True)
    name = "9a0f30ae3fc3"
    interval = "1s"
    print("Started podman stats...", file=sys.stderr)
    os.system("podman stats --format JSON %s" % (name))


def readProcess(q: multiprocessing.Queue):
    """ 创建管道，将管道输入端重定向到stdin，将输出端递交给statsProcess

    从管道中读取容器状态，得到内存占用(KB)，并从Date.now()获取时间一并存放到 q 中 """
    pin, pout = os.pipe()
    # print("readProcess: pin=%d, pout=%d" % (pin, pout))
    stdinFd = os.sys.stdin.fileno()
    os.dup2(pin, stdinFd)
    statP = multiprocessing.Process(target=statsProcess, args=(pout,))
    statP.start()
    while True:
        time.sleep(1)
        line = ""
        for i in range(0, 12):
            line += sys.stdin.readline()
        data = json.loads(line)[0]
        data['date'] = datetime.now().__str__()[5:19]

        tmp = data['mem_usage'].find("/")
        data['mem_usage'] = data['mem_usage'][0:tmp - 3]
        tmp = data['netio'].find("/")
        data['netio'] = data['netio'][0:tmp - 3]
        if data["cpu_percent"] == '--':
            data["cpu_percent"] = "0"
        else:
            tmp = len(data['cpu_percent'])
            data['cpu_percent'] = data['cpu_percent'][0:tmp - 1]
        if data["blocki"].startswith("--"):
            data["blocki"] = "0"
        q.put(data)
        # line = sys.stdin.readline()
        # data = json.loads(line)
        # res = data["data"]["memory"]["usage"]["usage"] / 1024
        # q.put({"date": datetime.now().__str__(), "data": res})
        # print(res, end=', ', flush=True)


def application(environ, start_response):
    path = environ['PATH_INFO']
    if path == "/state" or path == "/memdata":
        start_response('200 OK', [('Content-Type', 'text/plain')])
        if stateQueue.empty():
            body = json.dumps(obj={"status": 0, "data": ""})
        else:
            body = json.dumps(obj={"status": 1, "data": stateQueue.get()})
    else:
        start_response('200 OK', [('Content-Type', 'text/html')])
        # body = open("main.html", mode="r", encoding="utf-8").read()
        body = open("index.html", mode="r", encoding="utf-8").read()
    return [body.encode('utf-8')]


def nothing(maxn: int):
    import math
    import random
    res = 0
    for i in range(0, maxn):
        res = res + random.random() * 230
        res = math.sin(res) + random.randint(0, 400)
    return res
