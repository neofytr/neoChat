import socket
import sys

server = socket.socket()

server.connect(('localhost', 4040))

server.send(f"SIGNUP {sys.argv[1]} {sys.argv[2]}\r\n".encode())
server.send(f"LOGIN {sys.argv[1]} {sys.argv[2]}\r\n".encode())
server.send(f"POLL\r\n".encode())
server.send(f"REQ neofyt {sys.argv[1]}\r\n".encode())

print(server.recv(4096))
print(server.recv(4096))
print(server.recv(4096))
print(server.recv(4096))