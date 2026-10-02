# TCP File Transfer

A C-based TCP networking project for transferring files between users through a server.

## Overview

This project implements a simple TCP client/server application in C. The project is split into separate modules for the server, users, and rooms:

- **`server.c`** — server-side networking and application logic.
- **`user.c` / `user.h`** — user/client-related functionality.
- **`room.c` / `room.h`** — room/session-related functionality.
- **`.vscode/`** — Visual Studio Code configuration.
- **`server.exe`** — a pre-built Windows executable included with the project.

The project is intended as a lightweight example of building a networked application using TCP sockets in C.

## Project Structure

```text
TCPFileTransfer/
├── .vscode/
│   ├── c_cpp_properties.json
│   ├── launch.json
│   └── settings.json
├── room.c
├── room.h
├── server.c
├── server.exe
├── user.c
└── user.h
```

## Requirements

### Windows

For compiling the project yourself, you will need:

- Windows
- A C compiler such as **GCC/MinGW** or **MSVC**
- TCP socket support (Windows provides this through Winsock)

If you only want to run the included executable, the pre-built `server.exe` can be used without compiling the source.

### Linux / macOS

The source may require small platform-specific changes because Windows networking uses **Winsock**, while Linux and macOS use POSIX sockets.

## Building

### GCC / MinGW

From the project directory, try:

```bash
gcc server.c room.c user.c -o server.exe -lws2_32
```

Then run:

```bash
server.exe
```

> **Note:** The exact compiler flags may need to be adjusted depending on the socket APIs used by the source code and your compiler setup.

### Visual Studio Code

The repository already contains a `.vscode` directory with C/C++ and launch configuration files.

Open the project folder in Visual Studio Code and use the configured build/debug options. Make sure a compatible C compiler is installed and available in your `PATH`.

## Running

Start the server first:

```text
server.exe
```

The server is responsible for accepting TCP connections and handling connected users/rooms.

Clients/users can then connect to the running server using the functionality provided by the user module.

Because the exact connection protocol and command set are defined by the source code, consult `server.c`, `user.c`, and the room module when extending or integrating the project.

## How It Works

At a high level, the application follows a client/server model:

```text
             TCP connection
       ┌────────────────────────┐
       │                        │
       ▼                        │
┌──────────────┐        ┌───────┴───────┐
│    User /    │  TCP   │               │
│    Client    ├────────►     Server     │
│              │        │               │
└──────────────┘        └───────┬───────┘
                                │
                         ┌──────┴──────┐
                         │    Rooms    │
                         │   / Users   │
                         └─────────────┘
```

TCP provides a reliable, ordered connection between the communicating endpoints. The server manages connected users and the room/session functionality implemented by the project.

## Files

### `server.c`

Contains the main server implementation, including the server-side networking logic.

### `user.c` / `user.h`

Contains the user-related structures and functions used by the application.

### `room.c` / `room.h`

Contains the room-related structures and functions used to organise users or sessions.

### `server.exe`

A compiled Windows executable supplied with the project.

## Development

When modifying the project, keep the responsibilities of the modules separated:

- Put server/network handling in `server.c`.
- Put user-related functionality in `user.c` and declarations in `user.h`.
- Put room-related functionality in `room.c` and declarations in `room.h`.

After making changes, rebuild the executable rather than relying on an older `server.exe`.

## Troubleshooting

### `gcc` is not recognized

Install a GCC distribution such as MinGW-w64 and add its `bin` directory to your system `PATH`.

### Winsock linker errors

On Windows, make sure the Winsock library is linked:

```bash
-lws2_32
```

### The server starts but clients cannot connect

Check:

1. The server is running.
2. The client is using the correct IP address.
3. The client is using the correct port.
4. Windows Firewall is not blocking the application.
5. Both endpoints are using the same protocol expected by the server.

For a local test, the server address is commonly:

```text
127.0.0.1
```

