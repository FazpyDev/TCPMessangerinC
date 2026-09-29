# TCP Messenger

A simple TCP-based chat server written in **C** using **Winsock2**.

This project is a learning project focused on understanding TCP networking, sockets, dynamic arrays, structs, pointers, and memory management in C.

The server allows multiple clients to connect, create private rooms using a password, and communicate with other clients inside the same room.

---

## Features

* TCP client/server communication
* Multiple simultaneous clients
* Password-based chat rooms
* Automatic room creation
* Clients can join existing rooms using a password
* Maximum room size of **100 clients**
* Clients can leave/disconnect from rooms
* Empty rooms are automatically deleted
* Dynamic arrays for users and rooms
* Uses `select()` to handle multiple clients without requiring a thread for each connection
* System messages for:

  * Joining a room
  * Creating a room
  * Users joining
  * Users disconnecting
  * Full rooms

---

## How It Works

The server maintains two main dynamic arrays:

### Users

Each connected client is represented by a `User`:

```c
typedef struct {
    SOCKET clientSocket;
    size_t connectedRoomID;
} User;
```

The user's `clientSocket` identifies their TCP connection.

`connectedRoomID` stores the room they are currently connected to.

If the user isn't in a room:

```c
connectedRoomID == SIZE_MAX
```

is used as a placeholder value.

---

### Rooms

Each chat room is represented by a `Room`:

```c
typedef struct {
    SOCKET *connectedClientSockets;
    size_t numberOfConnectedSockets;
    size_t roomID;
    char password[100];
} Room;
```

A room contains:

* Its unique ID
* Its password
* An array of connected client sockets
* The number of clients currently connected

The room does **not** own the sockets themselves. The sockets are owned by the connected users; the room simply stores their socket handles so messages can be broadcast to them.

---

## Room System

When a client first connects, the server asks:

```text
[SYSTEM] Enter Room Password:
```

The client then enters a password.

### Existing room

If a room with that password already exists, the client joins it.

```text
Client A → password123
Client B → password123
```

Both clients will be placed into the same room.

Messages from one client are then sent to the other clients in that room.

### New room

If no room has that password, the server creates a new room using the supplied password.

For example:

```text
Client → myPassword

Room created:
ID: 1
Password: myPassword
```

Another client entering:

```text
myPassword
```

will join the existing room.

---

## Server Architecture

The server uses a single main loop with `select()` rather than creating a thread for every client.

The basic flow is:

```text
                 ┌──────────────┐
                 │    Server    │
                 │    :27015    │
                 └──────┬───────┘
                        │
              ┌─────────┴─────────┐
              │                   │
          Client A             Client B
              │                   │
              └─────────┬─────────┘
                        │
                    Room #1
                  "password123"
```

`select()` monitors:

* The listening socket
* Every connected client socket

When a socket becomes ready, the server determines what happened.

### New connection

```text
listen_socket → accept()
              → create User
              → add User to users[]
```

### Client message

```text
client socket → recv()
              → determine user's room
              → send message to room
```

### Client disconnect

```text
client socket
      ↓
remove socket from room
      ↓
delete room if empty
      ↓
remove User from users[]
      ↓
closesocket()
```

---

## Dynamic Memory

The project uses dynamic arrays for both users and rooms.

### Users

```c
User *users = NULL;
size_t usersLength = 0;
```

When a client connects, the array is expanded using `realloc()`.

When a client disconnects, the final user in the array is moved into the removed user's position:

```text
Before:

[User A] [User B] [User C] [User D]

Remove User B:

[User A] [User D] [User C]
```

This avoids having to shift every element after the removed user.

---

### Rooms

Rooms are also stored in a dynamic array:

```c
Room *rooms = NULL;
size_t roomsLength = 0;
```

Each room additionally owns its own dynamic array of socket handles:

```c
room.connectedClientSockets
```

When a client joins:

```text
[number of sockets + 1]
```

space is allocated.

When a client leaves, the socket is removed and the array is resized.

When the final client leaves a room, the room is deleted.

---

## Building

This project currently uses **Winsock2**, so it is designed for Windows.

You need:

* Windows
* GCC / MinGW
* Winsock2
* `ws2_32` library

The project can be compiled with:

```bash
gcc server.c -o server.exe -lws2_32
```

---

## Running the Server

Start the server:

```bash
./server.exe
```

The server listens on:

```text
127.0.0.1:27015
```

---

## Connecting a Client

For testing, [Ncat](https://nmap.org/ncat/) can be used as a simple TCP client.

```bash
ncat 127.0.0.1 27015
```

Open multiple terminals and run the same command in each one.

For example:

### Terminal 1

```text
ncat 127.0.0.1 27015

[SYSTEM] Enter Room Password:
secret
[SYSTEM] Room not found, new room created.
```

### Terminal 2

```text
ncat 127.0.0.1 27015

[SYSTEM] Enter Room Password:
secret
[SYSTEM] Room Found! You have Joined the room.
```

Messages sent by one client can then be received by the other client.

---

## Example

Suppose three clients connect:

```text
Client A → password: abc
Client B → password: abc
Client C → password: xyz
```

The server creates:

```text
Room #1
Password: abc
    ├── Client A
    └── Client B

Room #2
Password: xyz
    └── Client C
```

If Client A sends:

```text
Hello!
```

the server broadcasts it to the other clients in Room #1.

Client C will not receive it because they are in a different room.

---

## Important Functions

| Function                 | Purpose                                           |
| ------------------------ | ------------------------------------------------- |
| `handleClientJoin()`     | Accepts a new TCP connection and creates a `User` |
| `handleClientLeave()`    | Removes a disconnected user                       |
| `findClientSocket()`     | Finds a socket inside a room                      |
| `removeClientSocket()`   | Removes a socket from a room                      |
| `removeClientfromRoom()` | Handles removing a user from their room           |
| `addRoom()`              | Adds a newly created room to the room array       |
| `createRoom()`           | Initializes a new room                            |
| `joinRoom()`             | Adds a user to a room                             |
| `deleteRoom()`           | Removes an empty room                             |
| `findRoomIndexbyID()`    | Finds a room using its ID                         |
| `sendMessageinRoom()`    | Sends a message to other clients in a room        |
| `sendSYSMessageinRoom()` | Sends a system message to every client in a room  |

---

## Technologies

* **C**
* **TCP/IP**
* **Winsock2**
* `select()`
* `fd_set`
* Dynamic memory allocation
* `malloc()` / `realloc()` / `free()`
* C structs
* Windows sockets

---

## Current Limitations

This is currently a learning/experimental implementation rather than a production-ready chat server.

Some areas that can be improved include:

* TCP message framing
* Handling messages larger than the receive buffer
* Handling partial `send()` operations
* Password security/hashing
* User authentication
* Usernames
* Persistent rooms
* Multiple servers/channels
* Proper error recovery
* Graceful server shutdown
* Cross-platform networking
* More robust memory/error handling
* Encryption/TLS

### TCP Message Framing

TCP is a byte stream rather than a message-based protocol.

Currently the server treats each `recv()` call as one complete message:

```c
recv(currentSocket, recvBuffer, DEFAULT_BUFFERSIZE - 1, 0);
```

A future version should implement a proper protocol, such as newline-delimited messages or length-prefixed packets.

---

## Future Plans

Possible future additions include:

* A proper C++ client
* Graphical user interface
* Usernames
* Direct messages
* Multiple rooms/channels
* Server lists
* User lists
* Persistent accounts
* Authentication
* Encrypted communication
* Cross-platform support
* A custom application-layer protocol
* Better packet/message handling

The long-term goal is to evolve this from a simple TCP networking experiment into a more complete Discord-like messaging application.

---

## Learning Goals

This project is primarily intended to practise:

```text
C
│
├── Structs
├── Pointers
├── Dynamic arrays
├── realloc()
├── malloc()
├── free()
├── Memory ownership
│
└── Networking
    ├── TCP
    ├── Sockets
    ├── bind()
    ├── listen()
    ├── accept()
    ├── recv()
    ├── send()
    └── select()
```

The project is being developed incrementally, with the goal of understanding how the underlying networking and memory-management systems work rather than relying on a high-level networking framework.
