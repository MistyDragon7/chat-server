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
