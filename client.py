import socket
import sys

if len(sys.argv) != 2:
    print("Usage: python3 client.py <filename>")
    sys.exit(1)

filename = sys.argv[1]

client_sock = socket.socket()

file_server_port = 4040
client_sock.connect(("127.0.0.1", file_server_port))

client_sock.send(f"NEO_FETCH: {filename}".encode())
print(client_sock.recv(4096).decode())

client_sock.close()