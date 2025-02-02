import socket
import sys

if len(sys.argv) != 3:
    print("Usage: python3 client.py <login> <password>")
    sys.exit(1)

client_sock = socket.socket()

file_server_port = 4040
client_sock.connect(("127.0.0.1", file_server_port))

client_sock.send(f"SIGNUP {sys.argv[1]} {sys.argv[2]}\r\n".encode())
client_sock.send(f"LOGIN {sys.argv[1]} {sys.argv[2]}\r\n".encode())
print(client_sock.recv(4096).decode())
print(client_sock.recv(4096).decode())
print(client_sock.recv(4096).decode())

client_sock.close()