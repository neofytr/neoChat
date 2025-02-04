import socket
import threading
import sys 

server_ip = 'localhost'
server_port = 4040

service_queue = []
service_queue_lock = threading.Lock()

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)


def listen_to_server():
    global service_queue
    while(True):
        msg = server.recv(4096)

def main():
    try:
        server.connect((server_ip, server_port))
        service_queue_thread = threading.Thread(target=listen_to_server)
    except Exception as e:
        print(f"Error connecting to the chat server: {e}")
        sys.exit(-1)
    

if __name__ == "__main__":
    main()