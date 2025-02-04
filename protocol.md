# Chat Service Protocol Specification

## Error Codes
- ERR 99: Bad request format
- ERR 98: Server error
- ERR 100: User not found in registered users
- ERR 101: User already logged in
- ERR 102: Incorrect password
- ERR 103: Username already exists
- ERR 104: User not logged in
- ERR 105: Receiver not logged in
- ERR 106: Requested user not on server

## Commands

### 1. Login
```
LOGIN <username> <password>
```

Authentication process:
1. Verifies username exists in registered users table (ERR 100 if not found)
2. Checks if user is not already logged in (ERR 101 if already logged in)
3. Validates password against registered users table (ERR 102 if incorrect)

Success Response:
- Adds entry to curr_login_table with username, password, user_fd
- Returns `OK_LOGGEDIN`

### 2. Signup
```
SIGNUP <username> <password>
```

Registration process:
1. Verifies username does not exist in registered users table (ERR 103 if exists)

Success Response:
- Adds entry to registered_users_table with username and password
- Returns `OK_SIGNEDUP`

### 3. Logout
```
LOGOUT <username>
```

Logout process:
1. Verifies user exists in curr_login_table (ERR 104 if not found)

Success Response:
- Removes username entry from curr_login_table
- Returns `OK_LOGGEDOUT`

### 4. Send Message
```
SEND <sender_username> <rcvr_username> msg
```

Message sending process:
1. Verifies sender is in curr_login_table (ERR 104 if not found)
2. Verifies receiver is in curr_login_table (ERR 105 if not found)

Success Response:
- Sends message to receiver in format: `MESG <mesg_len> <msg>`
- Returns `OK_SENT` to sender

### 5. Poll Logged In Users
```
POLL
```

Response:
- Returns list of all users in curr_login_table
- Format: `USERS <username_one> <username_two> ...`

### 6. Chat Request
```
REQ <requested_username> <requesting_user>
```

Request process:
1. Verifies requested user is on server (ERR 106 if not found)

Success Response:
- Forwards to requested user: `CHAT_REQ <requesting_username>`

### 7. Chat Response
```
CHAT_RESP <requesting_username> ACCEPT/REJECT
```

Response handling:
- Sends `CHAT_ACCEPTED` or `CHAT_REJECTED <requesting_username>` based on response
- Handles related errors with appropriate error codes