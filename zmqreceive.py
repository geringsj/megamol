import time
import zmq

context = zmq.Context()
socket = context.socket(zmq.SUBSCRIBE)
socket.connect("tcp://127.0.0.1:12346")
print("start")

while True:
    print("loop")
    #  Wait for next request from client
    message = socket.recv()
    print("Received request: %s" % message)