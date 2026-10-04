# Simple Linux Chat Application

A terminal-based multi-client chat system built for Linux using **C++17**, **TCP sockets**, **POSIX threads**, **SQLite3**, and an optional **Linux character device driver**.

The project is designed as an educational Linux system-programming project. It demonstrates how a user-space C++ application can combine networking, concurrency, persistent storage, authentication, and a small kernel-space interface.

> **Project status:** The source tree contains the server, client, database layer, tests, documentation, build files, and the optional `chat_monitor` kernel module. The packaged project documentation states that full compilation, live multi-client testing, and driver loading were still pending at the time the project was assembled. Do not claim those parts are working until they have been tested on the target machine.

---

## Features

### Networking
- TCP client-server communication.
- Configurable server port.
- Multiple clients can connect at the same time.
- Server creates a separate thread for each connected client.
- Client uses a background receiver thread so incoming messages can appear while the user is typing.
- Line-based text protocol over TCP.

### User Accounts
- User registration.
- User login/logout.
- Username validation.
- Password hashing with SHA-256 before storage.
- Online/offline status tracking.
- First registered user automatically becomes an administrator.

### Messaging
- Broadcast messages to online users.
- Private messages between users.
- Room-based messaging.
- Timestamps on messages.
- Persistent message history.
- `/history` displays recent messages relevant to the logged-in user.

### Chat Rooms
- Create rooms.
- Join rooms.
- Leave rooms.
- List available rooms.
- Plain text is sent to the current room after joining it.

### Administration
- First registered account receives administrator privileges.
- Administrator can request a user to be kicked using `/kick username`.
- `/stats` reports the number of currently online users.

### Persistence
SQLite is used as a local database for:
- Users
- Rooms
- Room membership
- Messages

The database is stored at:

```text
database/chat.db
```

### Linux Kernel Integration
An optional Linux character device driver creates:

```text
/dev/chat_monitor
```

The server writes simple runtime statistics to this device when the driver is available.

Example:

```text
status=running
active_clients=2
total_messages=10
total_connections=5
```

The server is deliberately designed to continue working if `/dev/chat_monitor` does not exist.

---

# Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Server and client implementation |
| Linux / Ubuntu | Target operating system |
| TCP/IP sockets | Client-server communication |
| Threads / `std::thread` | Concurrent clients and client receiving |
| SQLite3 | Persistent data storage |
| SHA-256 | Educational password hashing |
| Linux character device | Server monitoring interface |
| GCC / G++ | Compilation |
| Git | Version control |

---

# System Architecture

The application is divided into four main areas:

```text
                    +----------------------+
                    |    Terminal Client   |
                    |      (C++17)         |
                    +----------+-----------+
                               |
                               | TCP
                               |
                    +----------v-----------+
                    |     Chat Server      |
                    |       (C++17)        |
                    +----------+-----------+
                               |
              +----------------+----------------+
              |                |                |
              v                v                v
       +-------------+  +-------------+  +-------------+
       | Auth / User |  | Room / Msg  |  |  Monitoring |
       | Management  |  | Management  |  | Integration |
       +------+------+  +------+------+  +------+------+
              |                |                |
              +----------------+                |
                               |                |
                        +------v------+         |
                        |   SQLite    |         |
                        | chat.db     |         |
                        +-------------+         |
                                               |
                                        +------v------+
                                        | /dev/       |
                                        | chat_monitor|
                                        +------+------+
                                               |
                                        +------v------+
                                        | Linux Kernel |
                                        | Character    |
                                        | Driver       |
                                        +-------------+
```

### Message flow

A typical message follows this path:

```text
Client
  |
  | send()
  v
TCP socket
  |
  v
Server client thread
  |
  +----> authenticate / identify sender
  |
  +----> broadcast / private message / room
  |
  +----> store message in SQLite
  |
  +----> update monitor statistics
  |
  v
Other connected clients
```

---

# Concurrency Model

The server accepts TCP connections and starts a dedicated thread for each client.

Conceptually:

```text
                    Server
                      |
              accept() connection
                      |
          +-----------+-----------+
          |           |           |
       Thread 1    Thread 2    Thread 3
          |           |           |
       Client A    Client B    Client C
```

The server protects its shared client list with `std::mutex`.

The client also has two concurrent activities:

```text
             Chat Client
                 |
        +--------+--------+
        |                 |
   Keyboard/input     Socket receiver
        |                 |
        v                 v
      send()           recv()
```

This allows incoming messages to be displayed without waiting for the user to enter a command.

---

# Project Structure

```text
SimpleChatApplication/
│
├── CMakeLists.txt
├── Makefile
├── README.md
│
├── bin/
│   ├── chat_server
│   └── chat_client
│
├── client/
│   ├── ChatClient.cpp
│   ├── ChatClient.h
│   ├── main.cpp
│   └── README.md
│
├── server/
│   ├── Authentication.cpp
│   ├── Authentication.h
│   ├── ChatServer.cpp
│   ├── ChatServer.h
│   ├── ClientHandler.cpp
│   ├── ClientHandler.h
│   ├── Database.cpp
│   ├── Database.h
│   ├── MessageManager.cpp
│   ├── MessageManager.h
│   ├── RoomManager.cpp
│   ├── RoomManager.h
│   ├── main.cpp
│   ├── chat_server
│   └── database/
│       └── chat.db
│
├── database/
│   └── chat.db
│
├── driver/
│   ├── chat_monitor.c
│   ├── Makefile
│   └── README.md
│
├── include/
│   ├── common.hpp
│   └── sha256.hpp
│
├── scripts/
│   ├── install_and_server.sh
│   └── wait_and_client.sh
│
├── tests/
│   ├── test_auth.cpp
│   ├── test_database.cpp
│   ├── test_message.cpp
│   └── README.md
│
└── docs/
    ├── introduction.md
    ├── requirements.md
    ├── architecture.md
    ├── design.md
    ├── testing.md
    ├── troubleshooting.md
    ├── progress.md
    └── presentation.md
```

### Main source modules

| Module | Responsibility |
|---|---|
| `ChatServer` | TCP listening, client management, broadcasting and server statistics |
| `ClientHandler` | Parses and executes client commands |
| `Authentication` | Registration, login, validation and password hashing |
| `Database` | SQLite connection, schema and database operations |
| `RoomManager` | Room creation, joining, leaving and listing |
| `MessageManager` | Message storage and history retrieval |
| `ChatClient` | TCP client connection, sending and receiving |
| `chat_monitor.c` | Linux character device for runtime statistics |
| `sha256.hpp` | SHA-256 implementation used by authentication |

---

# Requirements

The project is intended for Linux.

Install the development tools on Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential g++ gcc make cmake git sqlite3 libsqlite3-dev pkg-config linux-headers-$(uname -r)
```

Verify the important tools:

```bash
g++ --version
gcc --version
make --version
cmake --version
sqlite3 --version
```

For the optional kernel driver, matching Linux kernel headers are required.

---

# Installation

Clone or copy the project to your preferred directory.

Example:

```bash
cd ~/Projects
```

If using Git:

```bash
git clone <repository-url> SimpleChatApplication
cd SimpleChatApplication
```

If the project was supplied as a ZIP, extract it and enter the extracted directory:

```bash
cd SimpleChatApplication
```

---

# Building the Application

## Recommended: Make

From the project root:

```bash
make
```

This builds:

```text
bin/chat_server
bin/chat_client
```

The Makefile uses:

```text
C++17
-Wall
-Wextra
-O2
-pthread
-lsqlite3
```

### Clean the build

```bash
make clean
```

### Build again

```bash
make
```

---


# Running the Server

From the project root:

```bash
./bin/chat_server 5000
```

The port is optional if using the default:

```text
5000
```

For example:

```bash
./bin/chat_server
```

or:

```bash
./bin/chat_server 5001
```

The server creates the `database` directory if necessary and uses:

```text
database/chat.db
```

for persistent storage.

Keep the server terminal running.

---

# Running the Client

Open another terminal.

From the project root:

```bash
./bin/chat_client 127.0.0.1 5000
```

For a server running on another machine:

```bash
./bin/chat_client <server-ip> <port>
```

Example:

```bash
./bin/chat_client 192.168.1.10 5000
```

The client will display:

```text
Connected. Type /help
```

---

# Running Multiple Clients

To demonstrate multi-client communication, open several terminals.

### Terminal 1

```bash
./bin/chat_server 5000
```

### Terminal 2

```bash
./bin/chat_client 127.0.0.1 5000
```

### Terminal 3

```bash
./bin/chat_client 127.0.0.1 5000
```

### Terminal 4

```bash
./bin/chat_client 127.0.0.1 5000
```

Each client has its own server-side thread.

---

# Using the Chat Application

After connecting, type:

```text
/help
```

The server supports the following commands.

| Command | Description |
|---|---|
| `/help` | Display available commands |
| `/register username password` | Create a user account |
| `/login username password` | Log in |
| `/users` | Display online users |
| `/msg username message` | Send a private message |
| `/broadcast message` | Broadcast a message to logged-in users |
| `/create_room name` | Create a chat room |
| `/join name` | Join a chat room |
| `/leave` | Leave the current room |
| `/rooms` | List available rooms |
| `/history` | Display recent relevant message history |
| `/stats` | Display server statistics |
| `/kick username` | Administrator-only user kick |
| `/logout` | Log out without disconnecting |
| `/quit` | Disconnect from the server |

Plain text that does not start with `/` is treated as a broadcast message.

If the user is currently inside a room, plain text is sent to that room instead.

---

# Example Session

## Register

```text
/register alice secret
```

First user response:

```text
OK registered as admin (first user)
```

The first registered user automatically receives administrator privileges.

Create another account:

```text
/register bob password123
```

Expected response:

```text
OK registered
```

---

## Login

```text
/login alice secret
```

Expected response:

```text
OK login
```

---

## Check online users

```text
/users
```

Example:

```text
Online: alice bob
```

---

## Broadcast

```text
/broadcast Hello everyone!
```

Or simply:

```text
Hello everyone!
```

when not inside a room.

---

## Private message

```text
/msg bob Hello Bob
```

---

## Create a room

```text
/create_room lobby
```

---

## Join a room

```text
/join lobby
```

Now normal text is sent to the room:

```text
Hello room!
```

---

## Leave a room

```text
/leave
```

---

## List rooms

```text
/rooms
```

---

## View message history

```text
/history
```

The application stores messages in SQLite and retrieves recent history relevant to the logged-in user.

---

## Check statistics

```text
/stats
```

Example:

```text
stats online=2
```

An administrator may see:

```text
stats online=2 (you are admin)
```

---

## Logout

```text
/logout
```

The TCP connection remains open, but the account is logged out.

---

## Quit

```text
/quit
```

The client disconnects from the server.

---

# Authentication and Administration

## Username rules

Usernames must:

- contain 3–20 characters,
- use letters, digits, or `_`.

Examples:

```text
alice
bob123
chat_admin
```

Invalid examples include usernames containing spaces or unsupported special characters.

## Password rules

Passwords must contain at least 3 characters.

## Password storage

The project does **not** store the raw password.

The implementation calculates:

```text
SHA256(username + ":" + password)
```

and stores the resulting hexadecimal digest.

> This is appropriate for an educational project but is **not a production-grade password storage scheme**. A real authentication system should use a slow password KDF such as Argon2id, scrypt, or bcrypt together with a unique salt.

## Administrator

The first account registered in an empty database becomes an administrator.

The administrator can use:

```text
/kick username
```

Non-admin users receive:

```text
ERROR admin only
```

---

# Chat Rooms

Rooms are stored persistently in SQLite.

A typical workflow is:

```text
/create_room programming
/join programming
Hello everyone!
/leave
```

Room names must be 2–20 characters and may contain:

- letters,
- digits,
- underscore.

Room membership is stored in the `room_members` table.

---

# SQLite Database

The database is normally:

```text
database/chat.db
```

The application automatically creates the required tables when the database is opened.

## Tables

### `users`

Stores account information:

```text
id
username
password_hash
created_at
status
is_admin
```

### `rooms`

Stores chat rooms:

```text
id
name
created_at
```

### `messages`

Stores messages:

```text
id
sender_id
receiver_id
room_id
message
created_at
```

A message can represent:
- a broadcast,
- a private message,
- a room message.

### `room_members`

Connects users and rooms:

```text
room_id
user_id
```

The combination of room and user is the primary key.

## Inspecting the database

If SQLite is installed:

```bash
sqlite3 database/chat.db
```

Inside SQLite:

```sql
.tables
```

For example:

```sql
SELECT * FROM users;
```

```sql
SELECT * FROM rooms;
```

```sql
SELECT * FROM messages;
```

Exit with:

```sql
.quit
```

---

# Linux Character Device Driver

The project contains an educational kernel module:

```text
driver/chat_monitor.c
```

It creates:

```text
/dev/chat_monitor
```

The device is used only for monitoring information.

It is **not** a networking driver and does not implement TCP.


# Building the Kernel Module

Make sure the current kernel headers are installed:

```bash
sudo apt install -y gcc make linux-headers-$(uname -r)
```

Enter the driver directory:

```bash
cd driver
```

Build:

```bash
make
```

This should produce:

```text
chat_monitor.ko
```

---

# Loading the Kernel Module

Load it with:

```bash
sudo insmod chat_monitor.ko
```

Check that it is loaded:

```bash
lsmod | grep chat_monitor
```

Check kernel messages:

```bash
dmesg | tail
```

Check the device:

```bash
ls -l /dev/chat_monitor
```

Read the current statistics:

```bash
cat /dev/chat_monitor
```

Expected initial output is similar to:

```text
status=stopped
active_clients=0
total_messages=0
total_connections=0
```

After the chat server starts and clients connect, the server can update the device.

---

# Unloading the Driver

When finished:

```bash
sudo rmmod chat_monitor
```

Clean the driver build:

```bash
make clean
```

> **Warning:** A kernel module executes in kernel space. A bug in the driver can destabilize or freeze the operating system. Only load the module on a system where you are comfortable testing kernel code.

---

# Testing

The project contains three small C++ test programs:

```text
tests/test_auth.cpp
tests/test_database.cpp
tests/test_message.cpp
```

Run all automated tests with:

```bash
make test
```

The tests cover:

### Authentication test

Checks:
- SHA-256 result,
- valid username validation,
- invalid username rejection,
- first-user registration,
- duplicate registration rejection,
- successful login,
- incorrect password rejection,
- second-user registration.

### Database test

Checks:
- SQLite database opening,
- user creation,
- user lookup,
- room creation,
- room membership,
- saving a message,
- retrieving history.

### Message manager test

Checks:
- message storage,
- history retrieval.

---

# Manual Integration Testing

After building, a basic manual test can be performed with two or more clients.

## Test 1 — Server startup

```bash
./bin/chat_server 5000
```

Expected:

- Server starts without an error.
- TCP port 5000 is listening.

Check:

```bash
ss -ltn | grep 5000
```

## Test 2 — Client connection

```bash
./bin/chat_client 127.0.0.1 5000
```

Expected:

```text
Connected. Type /help
```

## Test 3 — Authentication

Register and log in:

```text
/register alice secret
/login alice secret
```

## Test 4 — Multiple users

Start a second client and register another account:

```text
/register bob secret
/login bob secret
```

From Alice:

```text
/users
```

Expected to show both users.

## Test 5 — Broadcast

Alice:

```text
/broadcast Hello Bob
```

Bob should receive the message.

## Test 6 — Private messaging

Alice:

```text
/msg bob Private hello
```

Bob should receive the private message.

## Test 7 — Room messaging

```text
/create_room lobby
/join lobby
Hello from the lobby
```

A second user can join the same room and receive room messages.

## Test 8 — History

```text
/history
```

Previously stored relevant messages should be displayed.

## Test 9 — Driver integration

With the driver loaded:

```bash
cat /dev/chat_monitor
```

Connect clients and send messages, then read the device again to verify that counters change.

---

# Troubleshooting

## `g++: command not found`

Install the compiler:

```bash
sudo apt update
sudo apt install -y build-essential g++
```

Then verify:

```bash
g++ --version
```

---

## `make: command not found`

Install:

```bash
sudo apt install -y make
```

---

## `sqlite3.h: No such file or directory`

Install the SQLite development package:

```bash
sudo apt install -y libsqlite3-dev
```

---

## `bind: Address already in use`

Another process is using port 5000.

Check:

```bash
ss -ltnp | grep 5000
```

Either stop the other program or choose another port:

```bash
./bin/chat_server 5001
```

Then connect with:

```bash
./bin/chat_client 127.0.0.1 5001
```

---

## `connect: Connection refused`

Make sure the server is running first:

```bash
./bin/chat_server 5000
```

Then start the client:

```bash
./bin/chat_client 127.0.0.1 5000
```

---

## `/dev/chat_monitor` does not exist

This normally means the optional driver has not been loaded.

Build and load it:

```bash
cd driver
make
sudo insmod chat_monitor.ko
```

Then:

```bash
ls -l /dev/chat_monitor
```

The main chat server does not require the driver to operate.

---

## Kernel module fails to build

Make sure headers matching the running kernel are installed:

```bash
uname -r
```

Then:

```bash
sudo apt install -y linux-headers-$(uname -r)
```

Retry:

```bash
cd driver
make
```

---

# Provided Scripts

The project contains helper scripts.

## `scripts/install_and_server.sh`

This script:

1. Updates APT.
2. Installs build dependencies.
3. Builds the project.
4. Starts the server on port 5000.

Before using it, verify that its hard-coded project path matches the actual location:

```text
/home/manish/Projects/SimpleChatApplication
```

If the project is stored elsewhere, edit the script accordingly.

Run:

```bash
chmod +x scripts/install_and_server.sh
./scripts/install_and_server.sh
```

## `scripts/wait_and_client.sh`

This script waits for port 5000 and then launches the client.

Run:

```bash
chmod +x scripts/wait_and_client.sh
./scripts/wait_and_client.sh
```

It also assumes the project is located at:

```text
/home/manish/Projects/SimpleChatApplication
```

Change the path if necessary.

---

# Security Notes

This project is primarily educational.

Important limitations include:

1. **SHA-256 is not a password KDF.** It is used here to demonstrate hashing. Production authentication should use Argon2id, scrypt, or bcrypt with salts.
2. **TCP traffic is not encrypted.** Passwords and messages are transmitted through the application protocol without TLS.
3. **The server is not hardened for hostile Internet exposure.**
4. Input validation is intentionally simple.
5. The `/kick` implementation requests socket shutdown; it is not a complete account/session management system.
6. There is no rate limiting or brute-force protection.
7. There is no secure session token mechanism.
8. The character driver is an educational kernel module and should not be treated as production kernel code.

For demonstrations, use localhost or a trusted local network.

---

# Design Decisions

## Why TCP?

TCP provides reliable, ordered byte-stream communication.

Chat messages should arrive in the order they were sent. TCP is therefore a straightforward choice for this educational project.

The application uses:

```text
socket()
bind()
listen()
accept()
connect()
send()
recv()
```

---

## Why threads?

The server uses one thread per connected client.

This prevents one client's blocking `recv()` call from stopping the server from handling other clients.

The client also uses a receiver thread so incoming messages can be printed while the user is entering commands.

---

## Why SQLite?

SQLite is:

- lightweight,
- file-based,
- easy to deploy,
- suitable for a small educational application,
- available through a standard C/C++ API.

No separate database server is required.

---

# Known Limitations

The current implementation should be considered an educational prototype rather than a production chat service.

Known limitations include:

- No persistent user sessions.
- No message delivery/read receipts.
- No file/image sharing.
- No GUI or web interface.
- No voice/video communication.
- No load balancing.
---

# Academic Project Mapping

The repository was organized around a Linux, Device Drivers, System Programming and C++ project.

| Academic Area | Project Evidence |
|---|---|
| C++ programming | C++17 client, server and manager classes |
| Linux system programming | POSIX sockets, file descriptors, `send()`, `recv()`, `open()`, `write()`, threads |
| Networking | TCP client-server architecture |
| Multithreading | Server client threads and client receiver thread |
| Database programming | SQLite3 integration |
| Authentication | Registration, login and password hashing |
| File/device interface | `/dev/chat_monitor` |
| Device drivers | `driver/chat_monitor.c` |
| Build systems | Makefile and CMake |
| Testing | C++ unit-style tests |
| Documentation | `docs/` directory and README |
| Version control | Git workflow documented in the project |

---

# Suggested Demonstration Flow

For a project presentation or viva, the following order provides a clear demonstration:

### 1. Show the project structure

```bash
tree
```

or:

```bash
find . -maxdepth 2 -type f
```

### 2. Build

```bash
make clean
make
```

### 3. Start the server

```bash
./bin/chat_server 5000
```

### 4. Start two or three clients

```bash
./bin/chat_client 127.0.0.1 5000
```

### 5. Demonstrate authentication

```text
/register alice secret
/login alice secret
```

### 6. Demonstrate messaging

```text
/broadcast Hello
/msg bob Private message
```

### 7. Demonstrate rooms

```text
/create_room linux
/join linux
Hello from the Linux room
```

### 8. Demonstrate history

```text
/history
```

### 9. Demonstrate administration

Log in as the first user and show:

```text
/kick bob
```

### 10. Demonstrate SQLite

```bash
sqlite3 database/chat.db
```

Then:

```sql
.tables
SELECT * FROM users;
SELECT * FROM rooms;
SELECT * FROM messages;
.quit
```

### 11. Demonstrate the kernel driver

```bash
cd driver
make
sudo insmod chat_monitor.ko
cat /dev/chat_monitor
```

### 12. Demonstrate live statistics

Connect clients and send messages, then:

```bash
cat /dev/chat_monitor
```

---

# Future Improvements

Potential future versions could add:

- Better session management.
- Message delivery status.
- Private room support.
- Message search.

---

## Project Documentation

Additional project documents are available under `docs/`:

- `docs/introduction.md` — project background and objectives
- `docs/requirements.md` — requirements document
- `docs/architecture.md` — architecture/design document
- `docs/design.md` — current implementation design
- `docs/testing.md` — testing plan and results
- `docs/troubleshooting.md` — common problems and fixes
- `docs/progress.md` — implementation progress
- `docs/presentation.md` — presentation/viva notes

The project also contains module-specific README files under:

```text
client/
server/
driver/
tests/
include/
```

---


```bash
./bin/chat_client 127.0.0.1 5000
```

### Run tests

```bash
make test
```

