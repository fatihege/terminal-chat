# Terminal Chat Application

A simple terminal-based chat application built with C++ using the Winsock2 library. This project implements a client-server architecture, allowing multiple clients to connect to a server and exchange messages in a group chat environment.

## Features

- **Client-Server Communication:** Uses TCP sockets for reliable communication.
- **Multi-Client Support:** Multiple clients can join the chat and send messages simultaneously.
- **Message Broadcasting:** Messages sent by a client are broadcasted to all other connected clients.
- **Graceful Shutdown:** The server can be stopped cleanly, closing all client connections.
- **Cross-Platform Code Structure:** The application is structured to make it easy to adapt to other platforms with minor changes.

---

## Prerequisites

- **CMake**: Version 3.30 or higher.
- **C++ Compiler**: Supports C++20 standard (e.g., GCC, MSVC, Clang).
- **Winsock2 Library**: Included in Windows development environments.

---

## Build Instructions

### 1. Clone the Repository
```bash
git clone https://github.com/fatihege/terminal-chat.git
cd terminal-chat
```

### 2. Configure the Project with CMake
```bash
cmake -B build
```

### 3. Build the Executables
```bash
cmake --build build
```

### 4. Run the Server and Client

- Start the Server:
- Start a Client:

---

## How to Use

### Starting the Server

1. Run the server executable:
    ```bash
    ./build/terminal_chat_server
    ```
2. The server will start listening for connections on port 8080 (by default).

### Connecting a Client

1. Run the client executable:
    ```bash
    ./build/terminal_chat_client
    ```
2. The client will connect to the server (default `127.0.0.1`).

### Sending Messages

- Type a message in the client terminal and press Enter to send it.
- Messages will be broadcasted to all connected clients.

### Stopping the Server
- Type `exit` and press Enter in the server terminal to stop the server gracefully.

---

## CMake Configuration

The **CMakeLists.txt** file is configured to build two separate executables: one for the server and one for the client.

### Key Points:

- Sets the C++ standard to C++20:
    ```cmake
    set(CMAKE_CXX_STANDARD 20)
    ```
- Links the Winsock2 library (ws2_32) to both executables:
    ```cmake
    target_link_libraries(terminal_chat_server ws2_32)
    target_link_libraries(terminal_chat_client ws2_32)
    ```