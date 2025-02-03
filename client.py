import tkinter as tk
from tkinter import ttk, messagebox
import socket
import threading
import time

class ChatClient:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Chat Application")
        self.root.geometry("400x500")
        
        self.style = ttk.Style()
        self.style.configure('TButton', padding=6)
        self.style.configure('TEntry', padding=3)
        
        self.client = None
        self.server_address = ('localhost', 4040)
        self.connected = False
        self.current_username = None
        
        self.main_frame = ttk.Frame(self.root, padding="10")
        self.main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        self.create_login_frame()
        self.create_signup_frame()
        self.create_message_frame()
        
        self.show_login_frame()
        
    def create_login_frame(self):
        self.login_frame = ttk.Frame(self.main_frame, padding="10")
        
        ttk.Label(self.login_frame, text="Login", font=('Arial', 16, 'bold')).grid(row=0, column=0, columnspan=2, pady=10)
        
        ttk.Label(self.login_frame, text="Username:").grid(row=1, column=0, pady=5)
        self.login_username = ttk.Entry(self.login_frame)
        self.login_username.grid(row=1, column=1, pady=5)
        
        ttk.Label(self.login_frame, text="Password:").grid(row=2, column=0, pady=5)
        self.login_password = ttk.Entry(self.login_frame, show="*")
        self.login_password.grid(row=2, column=1, pady=5)
        
        ttk.Button(self.login_frame, text="Login", command=self.handle_login).grid(row=3, column=0, columnspan=2, pady=10)
        ttk.Button(self.login_frame, text="Go to Signup", command=self.show_signup_frame).grid(row=4, column=0, columnspan=2)
        
    def create_signup_frame(self):
        self.signup_frame = ttk.Frame(self.main_frame, padding="10")
        
        ttk.Label(self.signup_frame, text="Sign Up", font=('Arial', 16, 'bold')).grid(row=0, column=0, columnspan=2, pady=10)
        
        ttk.Label(self.signup_frame, text="Username:").grid(row=1, column=0, pady=5)
        self.signup_username = ttk.Entry(self.signup_frame)
        self.signup_username.grid(row=1, column=1, pady=5)
        
        ttk.Label(self.signup_frame, text="Password:").grid(row=2, column=0, pady=5)
        self.signup_password = ttk.Entry(self.signup_frame, show="*")
        self.signup_password.grid(row=2, column=1, pady=5)
        
        ttk.Button(self.signup_frame, text="Sign Up", command=self.handle_signup).grid(row=3, column=0, columnspan=2, pady=10)
        ttk.Button(self.signup_frame, text="Back to Login", command=self.show_login_frame).grid(row=4, column=0, columnspan=2)
        
    def create_message_frame(self):
        self.message_frame = ttk.Frame(self.main_frame, padding="10")
        
        ttk.Label(self.message_frame, text="Send Message", font=('Arial', 16, 'bold')).grid(row=0, column=0, columnspan=2, pady=10)
        
        ttk.Label(self.message_frame, text="To:").grid(row=1, column=0, pady=5)
        self.receiver_username = ttk.Entry(self.message_frame)
        self.receiver_username.grid(row=1, column=1, pady=5)
        
        ttk.Label(self.message_frame, text="Message:").grid(row=2, column=0, pady=5)
        self.message_text = tk.Text(self.message_frame, height=4, width=30)
        self.message_text.grid(row=2, column=1, pady=5)
        
        ttk.Button(self.message_frame, text="Send", command=self.handle_send).grid(row=3, column=0, columnspan=2, pady=10)
        ttk.Button(self.message_frame, text="Logout", command=self.handle_logout).grid(row=4, column=0, columnspan=2)
        
    def ensure_connection(self):
        if not self.connected or self.client is None:
            try:
                if self.client:
                    self.client.close()
                self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.client.connect(self.server_address)
                self.connected = True
                return True
            except Exception as e:
                self.connected = False
                messagebox.showerror("Connection Error", f"Could not connect to server: {str(e)}")
                return False
        return True

    def send_and_receive(self, request):
        max_retries = 3
        retry_delay = 1  # seconds
        
        for attempt in range(max_retries):
            try:
                if not self.ensure_connection():
                    continue
                    
                self.client.send(request.encode())
                response = self.client.recv(1024).decode().strip()
                return response
                
            except (ConnectionError, socket.error) as e:
                self.connected = False
                if attempt < max_retries - 1:
                    time.sleep(retry_delay)
                    continue
                else:
                    raise ConnectionError(f"Failed to communicate with server after {max_retries} attempts")
                    
    def show_login_frame(self):
        self.signup_frame.grid_remove()
        self.message_frame.grid_remove()
        self.login_frame.grid(row=0, column=0)
        
    def show_signup_frame(self):
        self.login_frame.grid_remove()
        self.message_frame.grid_remove()
        self.signup_frame.grid(row=0, column=0)
        
    def show_message_frame(self):
        self.login_frame.grid_remove()
        self.signup_frame.grid_remove()
        self.message_frame.grid(row=0, column=0)
        
    def handle_login(self):
        username = self.login_username.get()
        password = self.login_password.get()
        
        if not username or not password:
            messagebox.showerror("Error", "Please fill in all fields")
            return
            
        try:
            request = f"LOGIN {username} {password}\n"
            response = self.send_and_receive(request)
            
            if response == "OK_LOGGEDIN":
                self.current_username = username
                messagebox.showinfo("Success", "Logged in successfully!")
                self.show_message_frame()
            elif response == "ERR 100":
                messagebox.showerror("Error", "User not found")
            elif response == "ERR 101":
                messagebox.showerror("Error", "User already logged in")
            elif response == "ERR 102":
                messagebox.showerror("Error", "Incorrect password")
            else:
                messagebox.showerror("Error", f"Unknown error: {response}")
                
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
            self.connected = False
            
    def handle_signup(self):
        username = self.signup_username.get()
        password = self.signup_password.get()
        
        if not username or not password:
            messagebox.showerror("Error", "Please fill in all fields")
            return
            
        try:
            request = f"SIGNUP {username} {password}\n"
            response = self.send_and_receive(request)
            
            if response == "OK_SIGNEDUP":
                messagebox.showinfo("Success", "Signed up successfully!")
                self.show_login_frame()
            elif response == "ERR 103":
                messagebox.showerror("Error", "Username already exists")
            else:
                messagebox.showerror("Error", f"Unknown error: {response}")
                
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
            self.connected = False
            
    def handle_send(self):
        receiver = self.receiver_username.get()
        message = self.message_text.get("1.0", tk.END).strip()
        
        if not receiver or not message:
            messagebox.showerror("Error", "Please fill in all fields")
            return
            
        try:
            request = f"SEND {self.current_username} {receiver} {message}\n"
            response = self.send_and_receive(request)
            
            if response == "OK_SENT":
                messagebox.showinfo("Success", "Message sent successfully!")
                self.message_text.delete("1.0", tk.END)
            elif response == "ERR 104":
                messagebox.showerror("Error", "Sender not logged in")
            elif response == "ERR 105":
                messagebox.showerror("Error", "Receiver not logged in")
            else:
                messagebox.showerror("Error", f"Unknown error: {response}")
                
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
            self.connected = False
            
    def handle_logout(self):
        try:
            request = f"LOGOUT {self.current_username}\n"
            response = self.send_and_receive(request)
            
            if response == "OK_LOGGEDOUT":
                messagebox.showinfo("Success", "Logged out successfully!")
                self.current_username = None
                self.connected = False
                if self.client:
                    self.client.close()
                self.client = None
                self.show_login_frame()
            elif response == "ERR 104":
                messagebox.showerror("Error", "User not logged in")
            else:
                messagebox.showerror("Error", f"Unknown error: {response}")
                
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
            self.connected = False
            
    def cleanup(self):
        if self.client:
            try:
                self.client.close()
            except:
                pass
        self.root.destroy()
        
    def run(self):
        self.root.protocol("WM_DELETE_WINDOW", self.cleanup)
        self.root.mainloop()

if __name__ == "__main__":
    app = ChatClient()
    app.run()