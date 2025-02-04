import socket
import threading
import sys
import select
import logging
import queue
import signal
import time
import os
from typing import Optional, Dict, List
from dataclasses import dataclass
from enum import Enum
from prompt_toolkit import PromptSession
from prompt_toolkit.shortcuts import clear
from prompt_toolkit.formatted_text import HTML

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
class ChatRoom:
    partner: str
    messages: List[tuple]  # List of (sender, message) tuples
    active: bool = True

class ChatClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.socket = None
        self.username = None
        self.logged_in = False
        self.stop_event = threading.Event()
        self.message_queue = queue.Queue()
        self.chat_rooms: Dict[str, ChatRoom] = {}
        self.active_chat_partner = None
        self.pending_requests = []
        self.online_users = []
        self.prompt_session = PromptSession()
        self.chat_request_event = threading.Event()
        
    def connect(self):
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            threading.Thread(target=self._listen_to_server, daemon=True).start()
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False

    def _listen_to_server(self):
        while not self.stop_event.is_set():
            try:
                readable, _, _ = select.select([self.socket], [], [], 1)
                if readable:
                    data = self.socket.recv(4096).decode()
                    if not data:
                        print("\nServer disconnected!")
                        self.stop_event.set()
                        break
                    messages = data.split('\r\n')
                    for msg in messages:
                        if msg:
                            self.message_queue.put(msg)
            except Exception as e:
                print(f"\nListener error: {e}")
                self.stop_event.set()
                break

    def send_to_server(self, message: str):
        try:
            self.socket.send(f"{message}\r\n".encode())
        except Exception as e:
            print(f"Send error: {e}")

    def handle_server_response(self, message: str):
        parts = message.split()
        msg_type = parts[0]

        if msg_type == "USERS":
            self.online_users = parts[1:]
        elif msg_type == "CHAT_REQ":
            self.pending_requests.append(parts[1])
            print(f"\nNew chat request from {parts[1]}!")
        elif msg_type == "CHAT_ACCEPTED":
            self.chat_request_event.set()
            partner = parts[1]
            self.chat_rooms[partner] = ChatRoom(partner=partner, messages=[])
            print(f"\nChat request accepted by {partner}!")
        elif msg_type == "CHAT_REJECTED":
            self.chat_request_event.set()
            print(f"\nChat request rejected by {parts[1]}")
        elif msg_type == "MESG":
            sender = parts[1]
            message = " ".join(parts[3:])
            if sender in self.chat_rooms:
                self.chat_rooms[sender].messages.append((sender, message))
                if sender == self.active_chat_partner:
                    print(f"\n{sender}: {message}")

    def show_menu(self):
        while not self.stop_event.is_set():
            clear()
            if not self.logged_in:
                print("\n=== Chat Application ===")
                print("1. Login")
                print("2. Signup")
                print("3. Exit")
                
                choice = input("\nEnter your choice: ")
                
                if choice == "1":
                    self.login_menu()
                elif choice == "2":
                    self.signup_menu()
                elif choice == "3":
                    self.stop_event.set()
                    break
            else:
                print(f"\n=== Welcome {self.username} ===")
                print("1. Start New Chat")
                print("2. View Chat Requests ({})".format(len(self.pending_requests)))
                print("3. Active Chats ({})".format(len(self.chat_rooms)))
                print("4. Poll Online Users")
                print("5. Logout")
                
                choice = input("\nEnter your choice: ")
                
                if choice == "1":
                    self.start_chat_menu()
                elif choice == "2":
                    self.handle_chat_requests()
                elif choice == "3":
                    self.show_active_chats()
                elif choice == "4":
                    self.poll_users()
                elif choice == "5":
                    self.logout()
                    
    def login_menu(self):
        username = input("Username: ")
        password = input("Password: ")
        self.send_to_server(f"LOGIN {username} {password}")
        time.sleep(0.5)  # Wait for server response
        if self.logged_in:
            self.username = username
            print("Login successful!")
        else:
            print("Login failed!")
        input("Press Enter to continue...")

    def signup_menu(self):
        username = input("Choose username: ")
        password = input("Choose password: ")
        self.send_to_server(f"SIGNUP {username} {password}")
        time.sleep(0.5)
        input("Press Enter to continue...")

    def start_chat_menu(self):
        self.poll_users()
        print("\nOnline Users:")
        for i, user in enumerate(self.online_users, 1):
            if user != self.username:
                print(f"{i}. {user}")
        
        choice = input("\nEnter user number to chat with (or 0 to cancel): ")
        if choice.isdigit() and 0 < int(choice) <= len(self.online_users):
            partner = self.online_users[int(choice)-1]
            if partner != self.username:
                self.send_to_server(f"REQ {partner} {self.username}")
                print(f"Chat request sent to {partner}. Waiting for response...")
                self.chat_request_event.clear()
                self.chat_request_event.wait(timeout=30)
                
        input("Press Enter to continue...")

    def handle_chat_requests(self):
        while self.pending_requests:
            requester = self.pending_requests[0]
            print(f"\nChat request from {requester}")
            choice = input("Accept? (y/n): ").lower()
            
            if choice == 'y':
                self.send_to_server(f"CHAT_RESP {requester} ACCEPT")
                self.chat_rooms[requester] = ChatRoom(partner=requester, messages=[])
            else:
                self.send_to_server(f"CHAT_RESP {requester} REJECT")
                
            self.pending_requests.pop(0)
        
        input("No more pending requests. Press Enter to continue...")

    def show_active_chats(self):
        if not self.chat_rooms:
            print("\nNo active chats!")
            input("Press Enter to continue...")
            return

        print("\nActive Chats:")
        for i, partner in enumerate(self.chat_rooms.keys(), 1):
            print(f"{i}. {partner}")
        
        choice = input("\nEnter chat number to open (or 0 to cancel): ")
        if choice.isdigit() and 0 < int(choice) <= len(self.chat_rooms):
            partner = list(self.chat_rooms.keys())[int(choice)-1]
            self.open_chat_room(partner)

    def open_chat_room(self, partner: str):
        chat_room = self.chat_rooms[partner]
        self.active_chat_partner = partner
        
        clear()
        print(f"\n=== Chat with {partner} ===")
        print("Type 'exit' to return to main menu")
        
        # Show chat history
        for sender, msg in chat_room.messages:
            print(f"{'You' if sender == self.username else sender}: {msg}")
        
        while chat_room.active and not self.stop_event.is_set():
            message = input(f"{self.username}> ")
            if message.lower() == 'exit':
                break
            
            self.send_to_server(f"SEND {self.username} {partner} {message}")
            chat_room.messages.append((self.username, message))
        
        self.active_chat_partner = None

    def poll_users(self):
        self.send_to_server("POLL")
        time.sleep(0.5)  # Wait for server response

    def logout(self):
        self.send_to_server(f"LOGOUT {self.username}")
        self.logged_in = False
        self.username = None
        self.chat_rooms.clear()
        self.pending_requests.clear()
        print("Logged out successfully!")
        input("Press Enter to continue...")

def main():
    client = ChatClient('localhost', 4040)
    
    if client.connect():
        try:
            client.show_menu()
        except KeyboardInterrupt:
            print("\nExiting...")
        finally:
            client.stop_event.set()
            if client.socket:
                client.socket.close()

if __name__ == "__main__":
    main()