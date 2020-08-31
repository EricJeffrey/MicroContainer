import multiprocessing
from wsgiref.simple_server import make_server
from resp import application
from resp import readProcess
from resp import stateQueue


def startServer():
    port = 8080
    httpd = make_server('', port, application)
    print("Serving Http on port %d..." % (port))
    httpd.serve_forever()


if __name__ == "__main__":
    readP = multiprocessing.Process(target=readProcess, args=(stateQueue,))
    readP.start()
    startServer()
