cmake_minimum_required(VERSION 3.10)
project(ChatServer CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(chat_server
    server/ChatServer.cpp
    server/main.cpp
)

target_include_directories(chat_server PRIVATE include)


add_executable(chat_client
    client/ChatClient.cpp
    client/main.cpp
)

target_include_directories(chat_client PRIVATE include)
