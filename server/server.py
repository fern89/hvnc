from flask import request, Flask, make_response
app = Flask(__name__)
import requests, struct
from requests.structures import CaseInsensitiveDict
import socket
from io import BytesIO
@app.route("/keydown/<key>" , methods=['GET'])
def kd(key):
    conn.sendall(b"\x69"+struct.pack("<I", int(key))+b"\x00\x00\x00\x00")
    return "a"
@app.route("/mouse/<x>/<y>" , methods=['GET'])
def click(x,y):
    conn.sendall(b"\x71"+struct.pack("<I", int(x))+struct.pack("<I", int(y)))
    return "a"
@app.route("/firstimage.png" , methods=['GET'])
def a2():
    conn.sendall(b"\x67"*9)
    sz = int.from_bytes(conn.recv(4, socket.MSG_WAITALL), "little")
    conn.recv(8, socket.MSG_WAITALL)
    data = conn.recv(sz, socket.MSG_WAITALL)
    response = make_response(data)
    response.headers.set('Content-Type', 'image/png')
    return response
@app.route("/image.png" , methods=['GET'])
def a():
    conn.sendall(b"\x68"*9)
    sz = int.from_bytes(conn.recv(4, socket.MSG_WAITALL), "little")
    x = int.from_bytes(conn.recv(4, socket.MSG_WAITALL), "little")
    y = int.from_bytes(conn.recv(4, socket.MSG_WAITALL), "little")
    data = conn.recv(sz, socket.MSG_WAITALL)
    response = make_response(data)
    response.headers.set('Content-Type', 'image/png')
    response.headers.set('X', str(x))
    response.headers.set('Y', str(y))
    return response
@app.route("/" , methods=['GET'])
def ma():
    f=open("demo.html", "r")
    r=f.read()
    f.close()
    return r
if __name__ == "__main__":
    HOST = "0.0.0.0"
    PORT = 6968
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    print("Connection received!")
    from waitress import serve
    serve(app, host="127.0.0.1", port=1337)
