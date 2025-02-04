import socket
import threading
import sys 
import select

server_ip = 'localhost'
server_port = 4040
connected = False

service_queue = []
service_queue_lock = threading.Lock()

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

stop_event = threading.Event()

def listen_to_server():
    global service_queue
    global service_queue_lock
    while not stop_event.is_set():
        try:
            readable, _, _ = select.select([server], [], [], 1)
            if readable:
                msg = server.recv(4096)
                if not msg:
                    print("Server closed the connection/Server went Offline!")
                    print("Exiting")
                    stop_event.set()
                    sys.exit(0)
                indiv_msgs = msg.split(b"\r\n")
                with service_queue_lock:
                    for msg in indiv_msgs:
                        if msg:
                            service_queue.append(msg)
                print(service_queue)
        except Exception as e:
            print(f"Error in listener thread: {e}")
            break
        

def main():
    global connected
    service_queue_thread = None
    
    try:
        try:
            server.connect((server_ip, server_port))
            connected = True
            
            service_queue_thread = threading.Thread(target=listen_to_server)
            service_queue_thread.start()
            
            server.send(b"SIGNUP neofyt rajrishika0220\r\n")
            server.send(b"LOGIN neofyt rajrishika0220\r\n")
            
            while not stop_event.is_set():
                pass
            
            print("\nClosing connection to the chat server...")
            server.close()
            print("\nGoodbye!")
            sys.exit(0)

     
        except Exception as e:
            print(f"Error connecting to the chat server: {e}")
            sys.exit(-1)
            
    except KeyboardInterrupt:
        print("\nKeyboard Interrupt detected")
        if connected:
            print("\nShutting down the server listening thread...")
            stop_event.set()
            if service_queue_thread:
                service_queue_thread.join()
            print("\nClosing connection to the chat server...")
            server.close()
            print("\nGoodbye!")
            sys.exit(0)

if __name__ == "__main__":
    main()