import socket
import threading
import sys
import time

class ChatClient:
    def __init__(self, host='localhost', port=4040):
        self.host = host
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.current_user = None
        self.is_chatting = False
        self.chat_partner = None
        self.chat_lock = threading.Lock()
        self.message_queue = []
        self.queue_lock = threading.Lock()
        
    def connect(self):
        try:
            self.socket.connect((self.host, self.port))
            threading.Thread(target=self.listen_for_messages, daemon=True).start()
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False
            
    def send_to_server(self, message):
        try:
            self.socket.send(f"{message}\r\n".encode())
            with self.queue_lock:
                if len(self.message_queue) > 0:
                    response = self.message_queue.pop(0)
                    return response
                    
            response = self.socket.recv(1024).decode().strip()
            return response
        except Exception as e:
            print(f"Error sending message: {e}")
            sys.exit(1)

    def signup(self):
        while True:
            username = input("Enter username: ").strip()
            if not username or ' ' in username:
                print("Invalid username. Username cannot be empty or contain spaces.")
                continue
                
            password = input("Enter password: ").strip()
            if not password or ' ' in password:
                print("Invalid password. Password cannot be empty or contain spaces.")
                continue
                
            response = self.send_to_server(f"SIGNUP {username} {password}")
            
            if response == "OK_SIGNEDUP":
                print("Successfully signed up!")
                return True
            elif response == "ERR 103":
                print("Username already exists!")
            elif response == "ERR 99":
                print("Invalid request format!")
            elif response == "ERR 98":
                print("Server error occurred!")
            else:
                print(f"Unexpected error: {response}")
            
            retry = input("Would you like to try again? (y/n): ")
            if retry.lower() != 'y':
                return False

    def login(self):
        while True:
            if self.current_user:
                print("Already logged in!")
                return True
                
            username = input("Enter username: ").strip()
            if not username or ' ' in username:
                print("Invalid username. Username cannot be empty or contain spaces.")
                continue
                
            password = input("Enter password: ").strip()
            if not password or ' ' in password:
                print("Invalid password. Password cannot be empty or contain spaces.")
                continue
                
            response = self.send_to_server(f"LOGIN {username} {password}")
            
            if response == "OK_LOGGEDIN":
                print("Successfully logged in!")
                self.current_user = username
                return True
            elif response == "ERR 100":
                print("User not registered!")
            elif response == "ERR 101":
                print("User already logged in!")
            elif response == "ERR 102":
                print("Incorrect password!")
            elif response == "ERR 99":
                print("Invalid request format!")
            elif response == "ERR 98":
                print("Server error occurred!")
            else:
                print(f"Unexpected error: {response}")
            
            retry = input("Would you like to try again? (y/n): ")
            if retry.lower() != 'y':
                return False

    def poll_users(self):
        if not self.current_user:
            print("Please login first!")
            return []
        
        response = self.send_to_server("POLL")
        if response.startswith("USERS"):
            users = response.split()[1:]  # Remove the "USERS" prefix
            if users:
                print("\nCurrently online users:")
                for user in users:
                    if user != self.current_user:
                        print(f"- {user}")
            else:
                print("No other users online")
            return users
        else:
            print(f"Error polling users: {response}")
            return []

    def handle_chat_request(self, requesting_user):
        if self.is_chatting:
            self.send_to_server(f"CHAT_RESP {requesting_user} REJECT")
            return
            
        print(f"\nChat request from {requesting_user}")
        choice = input("Accept chat? (y/n): ").lower()
        
        if choice == 'y':
            self.send_to_server(f"CHAT_RESP {requesting_user} ACCEPT")
            with self.chat_lock:
                self.is_chatting = True
                self.chat_partner = requesting_user
            print(f"\nStarting chat with {requesting_user}")
        else:
            self.send_to_server(f"CHAT_RESP {requesting_user} REJECT")

    def start_chat_session(self, partner):
        with self.chat_lock:
            self.is_chatting = True
            self.chat_partner = partner
        
        print("\nChat session started")
        print("Type 'exit' to end chat")
        
        while self.is_chatting:
            try:
                message = input(f"{self.current_user}: ").strip()
                if not message:
                    continue
                    
                if message.lower() == 'exit':
                    with self.chat_lock:
                        self.is_chatting = False
                        self.chat_partner = None
                    print("Chat session ended")
                    break
                    
                response = self.send_to_server(f"SEND {self.current_user} {partner} {message}")
                if response == "ERR 104":
                    print("You are not logged in!")
                    break
                elif response == "ERR 105":
                    print("Chat partner is no longer online!")
                    break
                elif response == "ERR 99":
                    print("Invalid message format!")
                elif response == "ERR 98":
                    print("Server error occurred!")
                    break
                elif response != "OK_SENT":
                    print(f"Unexpected error: {response}")
                    break
                    
            except Exception as e:
                print(f"Error in chat session: {e}")
                break
        
        with self.chat_lock:
            self.is_chatting = False
            self.chat_partner = None

    def chat_with_user(self):
        if not self.current_user:
            print("Please login first!")
            return
            
        if self.is_chatting:
            print("Already in a chat session!")
            return
            
        users = self.poll_users()
        if not users:
            return
            
        target_user = input("\nEnter username to chat with: ").strip()
        if not target_user or target_user == self.current_user:
            print("Invalid user selection!")
            return
            
        if target_user not in users:
            print("User not online!")
            return
            
        response = self.send_to_server(f"REQ {target_user} {self.current_user}")
        
        if response == "ERR 106":
            print("User not online!")
        elif response == "OK_SENT":
            print(f"Chat request sent to {target_user}, waiting for response...")
        else:
            print(f"Error sending chat request: {response}")

    def listen_for_messages(self):
        while True:
            try:
                data = self.socket.recv(1024).decode().strip()
                if not data:
                    print("\nDisconnected from server")
                    sys.exit(1)
                
                if data.startswith("MESG"):
                    _, msg_len, *msg_parts = data.split(maxsplit=2)
                    message = msg_parts[0] if msg_parts else ""
                    if self.is_chatting:
                        print(f"\n{self.chat_partner}: {message}")
                        print(f"{self.current_user}: ", end='', flush=True)
                
                elif data.startswith("CHAT_REQ"):
                    requesting_user = data.split()[1]
                    self.handle_chat_request(requesting_user)
                
                elif data.startswith("CHAT_ACCEPTED"):
                    accepted_user = data.split()[1]
                    print(f"\nChat request accepted by {accepted_user}")
                    self.start_chat_session(accepted_user)
                
                elif data.startswith("CHAT_REJECTED"):
                    rejected_user = data.split()[1]
                    print(f"\nChat request rejected by {rejected_user}")
                    with self.chat_lock:
                        self.is_chatting = False
                        self.chat_partner = None
                
                else:
                    with self.queue_lock:
                        self.message_queue.append(data)
                
            except Exception as e:
                print(f"\nConnection error: {e}")
                sys.exit(1)

    def logout(self):
        if not self.current_user:
            print("Not logged in!")
            return True
            
        if self.is_chatting:
            print("Please exit the current chat session first!")
            return False
            
        response = self.send_to_server(f"LOGOUT {self.current_user}")
        if response == "OK_LOGGEDOUT":
            print("Successfully logged out!")
            self.current_user = None
            return True
        elif response == "ERR 104":
            print("Not logged in!")
            self.current_user = None
            return True
        else:
            print(f"Error logging out: {response}")
            return False

    def main_menu(self):
        while True:
            print("\n=== Chat Client Menu ===")
            print("1. Sign up")
            print("2. Login")
            print("3. Poll for users")
            print("4. Chat with user")
            print("5. Logout")
            print("6. Exit")
            
            choice = input("\nEnter your choice (1-6): ").strip()
            
            if choice == '1':
                self.signup()
            elif choice == '2':
                self.login()
            elif choice == '3':
                self.poll_users()
            elif choice == '4':
                self.chat_with_user()
            elif choice == '5':
                self.logout()
            elif choice == '6':
                if self.current_user:
                    self.logout()
                print("Goodbye!")
                sys.exit(0)
            else:
                print("Invalid choice!")

def main():
    client = ChatClient()
    if client.connect():
        try:
            client.main_menu()
        except KeyboardInterrupt:
            if client.current_user:
                client.logout()
            print("\nGoodbye!")
            sys.exit(0)
    else:
        print("Failed to connect to server")

if __name__ == "__main__":
    main()