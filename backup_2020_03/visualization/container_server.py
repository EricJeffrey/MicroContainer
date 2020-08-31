from wsgiref.simple_server import make_server
import os
import sys


def application(environ, start_response):
    tmpPath = environ["PATH_INFO"]
    if os.path.exists(tmpPath):
        start_response('200 OK', [('Content-Type', 'text/plain')])
        fp = open(tmpPath, mode="rb")
        return [fp.read()]
    else:
        start_response('404 NOF_FOUND', [('Content-Type', 'text/plain')])
        return ["Not Found!".encode('utf-8')]


def startServer():
    httpd = make_server('', 8000, application)
    print("Server Started...")
    httpd.serve_forever()


startServer()
