// include/ChatServer.hpp

#ifndef CHAT_SERVER_HPP
#define CHAT_SERVER_HPP

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define closesocket close
#endif

#include <string>
#include <mutex>
#include <map> // For std::map

#include "user/UserManager.hpp" // Include UserManager

class ChatServer
{
public:
    ChatServer(int port);
    ~ChatServer();
    void start();

private:
    void accept_clients();
    void handle_client(int client_socket);
    void broadcast(const std::string &message, int sender_socket);
    void remove_client(int socket);

    // Helper function to handle chat commands (e.g., /friend, /msg)
    void process_chat_command(int client_socket, const std::string& sender_username, const std::string& message);

    int port_;
    int server_fd_;
    std::map<int, std::string> clients_; // Change clients_ to map socket to username
    std::mutex clients_mutex_;
    bool running_ = false;
    UserManager user_manager_;
};

#endif // CHAT_SERVER_HPP