import socket

server = socket.socket()

server.connect(('localhost', 4040))

server.send(b"SIGNUP neofyt rajrishika0220\r\n")
server.send(b"LOGIN neofyt rajrishika0220\r\n")
server.send(b"POLL\r\n")
print(server.recv(4096))
print(server.recv(4096))
print(server.recv(4096))