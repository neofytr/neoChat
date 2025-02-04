import socket
import threading
import sys
import select
import logging
import queue
import signal
from typing import Optional
from dataclasses import dataclass
from enum import Enum

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

class ChatError(Exception):
    pass

class ErrorCodes(Enum):
    USER_NOT_REGISTERED = "100"
    USER_ALREADY_LOGGED_IN = "101"
    INVALID_PASSWORD = "102"
    USER_ALREADY_EXISTS = "103"
    USER_NOT_LOGGED_IN = "104"
    RECIPIENT_NOT_LOGGED_IN = "105"
    REQUESTED_USER_NOT_FOUND = "106"

class MessageType(Enum):
    SIGNUP = "SIGNUP"
    LOGIN = "LOGIN"
    LOGOUT = "LOGOUT"
    SEND = "SEND"
    POLL = "POLL"
    REQ = "REQ"
    CHAT_RESP = "CHAT_RESP"
    MESG = "MESG"
    USERS = "USERS"
    CHAT_REQ = "CHAT_REQ"
    CHAT_ACCEPTED = "CHAT_ACCEPTED"
    CHAT_REJECTED = "CHAT_REJECTED"
    ERR = "ERR"
    OK = "OK"

@dataclass
class ServerConfig:
    host: str
    port: int
    buffer_size: int = 4096
    encoding: str = 'utf-8'

class ChatClient:
    def __init__(self, config: ServerConfig):
        self.config = config
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.message_queue = queue.Queue()
        self.stop_event = threading.Event()
        self.listener_thread: Optional[threading.Thread] = None
        self.username: Optional[str] = None
        self.password: Optional[str] = None
        self.logged_in = False
        
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _signal_handler(self, signum, frame):
        logging.info("Received shutdown signal")
        self.shutdown()

    def connect(self) -> None:
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.config.host, self.config.port))
            self.connected = True
            logging.info("Successfully connected to server")
        except Exception as e:
            raise ChatError(f"Failed to connect to server: {e}")

    def send_message(self, msg_type: MessageType, content: str) -> None:
        if not self.connected or not self.socket:
            raise ChatError("Not connected to server")
        
        try:
            message = f"{msg_type.value} {content}\r\n"
            self.socket.send(message.encode(self.config.encoding))
        except Exception as e:
            raise ChatError(f"Failed to send message: {e}")

    def _listen_to_server(self) -> None:
        while not self.stop_event.is_set() and self.socket:
            try:
                readable, _, _ = select.select([self.socket], [], [], 1)
                
                if readable:
                    data = self.socket.recv(self.config.buffer_size)
                    
                    if not data:
                        logging.error("Server disconnected")
                        self.shutdown()
                        break
                    
                    messages = data.decode(self.config.encoding).split('\r\n')
                    for msg in messages:
                        if msg.strip():
                            self.message_queue.put(msg)
                            logging.info(f"Received: {msg}")
                            
            except Exception as e:
                logging.error(f"Error in listener thread: {e}")
                self.shutdown()
                break

    def start_listening(self) -> None:
        self.listener_thread = threading.Thread(target=self._listen_to_server)
        self.listener_thread.daemon = True
        self.listener_thread.start()

    def shutdown(self) -> None:
        if not self.stop_event.is_set():
            self.stop_event.set()
            
            if self.connected and self.socket:
                try:
                    self.send_message(MessageType.LOGOUT, "")
                except ChatError:
                    pass
                
                self.socket.close()
                self.connected = False
            
            if self.listener_thread and self.listener_thread.is_alive():
                self.listener_thread.join(timeout=2)
            
            logging.info("Client shutdown complete")

def main():
    config = ServerConfig(
        host='localhost',
        port=4040
    )
    
    client = ChatClient(config)
    
    try:
        client.connect()
        client.start_listening()
        
        client.send_message(MessageType.SIGNUP, "neofyt rajrishika0220")
        client.send_message(MessageType.LOGIN, "neofyt rajrishika0220")
        
        while not client.stop_event.is_set():
            try:
                while not client.message_queue.empty():
                    message = client.message_queue.get_nowait()
                    print(f"Processing message: {message}")
                    
            except queue.Empty:
                pass
            
    except ChatError as e:
        logging.error(f"Chat error occurred: {e}")
    except Exception as e:
        logging.error(f"Unexpected error: {e}")
    finally:
        client.shutdown()

if __name__ == "__main__":
    main()