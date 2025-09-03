# Chat Server

A simple C++ chat application featuring a server and client, designed to demonstrate real-time communication.

## Features

*   **Real-time Messaging:** Exchange messages instantly between connected clients.
*   **Multi-client Support:** Handles multiple concurrent client connections.
*   **Command-line Interface:** Simple text-based interface for both server and client.
*   **JSON Communication:** Uses JSON for structured message exchange between server and client.

## Prerequisites

Before you begin, ensure you have the following installed:

*   **CMake:** Version 3.10 or higher.
*   **C++ Compiler:**
    *   **Windows:** Visual Studio (e.g., Visual Studio 2022)
    *   **Linux/macOS:** GCC or Clang (C++17 or later recommended)
*   **Git** (optional, for cloning the repository)

## Building the Project

This project uses CMake for cross-platform building.

### Cloning the Repository (Optional)

```bash
git clone https://github.com/your-username/chat-server.git
cd chat-server
```

### Windows

To build on Windows with Visual Studio (e.g., Visual Studio 2022):

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" # Or your installed Visual Studio version
cmake --build .
```

### Linux/macOS

To build on Linux or macOS:

```bash
mkdir -p build
cd build
cmake ..
make
```

## Running the Application

### Running the Server

Open a terminal and navigate to the `build` directory:

```bash
cd build
./chat_server # Linux/macOS
.\Debug\chat_server.exe # Windows (or Release if built in Release mode)
```

### Running the Client

Open another terminal and navigate to the `build` directory:

```bash
cd build
./chat_client # Linux/macOS
.\Debug\chat_client.exe # Windows (or Release if built in Release mode)
```

## Client Commands

Clients can interact with the server using the following commands:

*   `/friend add <username>`: Sends a friend request to the specified user.
*   `/friend accept <username>`: Accepts a pending friend request from the specified user.
*   `/friend reject <username>`: Rejects a pending friend request from the specified user.
*   `/msg <username> <message>`: Sends a direct message to the specified user.
*   `/quit`: Disconnects from the chat server.
*   `/pending`: Lists all incoming pending friend requests.

## Docker Setup

You can also run the server and client using Docker.

### Build Docker Images

```bash
docker build -t chat-server-image .
```

### Run Server in Docker

```bash
docker run -p 12345:12345 -d --name chat-server chat-server-image ./chat_server
```

### Run Client in Docker

```bash
docker run -it --rm --network host chat-server-image ./chat_client
```

## Project Structure

```
.
├── client/                 # Client-side source code
│   ├── ChatClient.cpp
│   └── main.cpp
├── include/                # Header files
│   ├── ChatClient.hpp
│   ├── ChatServer.hpp
│   ├── Color.hpp
│   ├── Common.hpp
│   ├── nlohmann/           # JSON library
│   │   └── json.hpp
│   └── user/
│       ├── User.hpp
│       └── UserManager.hpp
├── server/                 # Server-side source code
│   ├── ChatServer.cpp
│   └── main.cpp
├── user/                   # User management source code
│   ├── User.cpp
│   └── UserManager.cpp
├── CMakeLists.txt          # CMake build configuration
├── Dockerfile              # Docker build file
├── LICENSE                 # Project license
└── README.md               # This README file
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
