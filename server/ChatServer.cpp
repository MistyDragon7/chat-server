// ChatServer.cpp (Cross-platform)
#include "../include/ChatServer.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

#include "../include/user/UserManager.hpp"
#include "../include/Color.hpp"
#include "../include/Common.hpp" // Include Common.hpp

// UserManager user_manager("users.json"); // Removed global UserManager

ChatServer::ChatServer(int port) : port_(port), server_fd_(-1), user_manager_("users.json") {}

ChatServer::~ChatServer()
{
    if (server_fd_ != -1)
    {
        CLOSE_SOCKET(server_fd_);
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void ChatServer::start()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        exit(EXIT_FAILURE);
    }
#endif

    struct sockaddr_in address;
    int opt = 1;
    socklen_t addlen = sizeof(address);

    server_fd_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (server_fd_ < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

#ifndef _WIN32
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
#else
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#endif

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd_, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    running_ = true;
    std::cout << "Server listening on port: " << port_ << std::endl;
    accept_clients();
}

void ChatServer::accept_clients()
{
    while (running_)
    {
        int client_socket = static_cast<int>(accept(server_fd_, nullptr, nullptr));
        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }
        std::thread(&ChatServer::handle_client, this, client_socket).detach();
    }
}

void ChatServer::handle_client(int client_socket)
{
    char buffer[1024];
    std::string received_data;

    // First, receive and validate the handshake magic string
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        CLOSE_SOCKET(client_socket);
        std::cerr << "Client disconnected during handshake or sent no data." << std::endl;
        return;
    }
    buffer[bytes_received] = '\0';
    std::string handshake_received(buffer);

    // Strip carriage return and newline from handshake
    if (!handshake_received.empty() && handshake_received.back() == '\r') {
        handshake_received.pop_back();
    }
    if (!handshake_received.empty() && handshake_received.back() == '\n') {
        handshake_received.pop_back();
    }

    if (handshake_received != CLIENT_HANDSHAKE_MAGIC.substr(0, CLIENT_HANDSHAKE_MAGIC.length() -1)) { // Compare without newline
        std::cerr << COLOR_RED << "Invalid handshake from client: " << handshake_received << COLOR_RESET << std::endl;
        CLOSE_SOCKET(client_socket);
        return;
    }

    // Read username and password
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        CLOSE_SOCKET(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';
    received_data += buffer;

    size_t username_end = received_data.find('\n');
    if (username_end == std::string::npos) {
        // Error: username not terminated by newline, or incomplete data
        CLOSE_SOCKET(client_socket);
        return;
    }
    std::string username = received_data.substr(0, username_end);
    // Strip carriage return and newline from username
    if (!username.empty() && username.back() == '\r') {
        username.pop_back();
    }
    if (!username.empty() && username.back() == '\n') {
        username.pop_back();
    }
    received_data.erase(0, username_end + 1);

    size_t password_end = received_data.find('\n');
    if (password_end == std::string::npos) {
        // Error: password not terminated by newline, or incomplete data
        CLOSE_SOCKET(client_socket);
        return;
    }
    std::string password = received_data.substr(0, password_end);
    // Strip carriage return and newline from password
    if (!password.empty() && password.back() == '\r') {
        password.pop_back();
    }
    if (!password.empty() && password.back() == '\n') {
        password.pop_back();
    }
    received_data.erase(0, password_end + 1);

    // Check if user exists, if not, try to register
    if (!user_manager_.userExists(username)) {
        if (user_manager_.registerUser(username, password)) {
            std::cout << "New user " << username << " registered successfully." << std::endl;
        } else {
            std::string reg_failed_msg = COLOR_RED "[Server]: Registration failed for user: " + username + ". Please try again." COLOR_RESET "\n";
            send(client_socket, reg_failed_msg.c_str(), static_cast<int>(reg_failed_msg.length()), 0);
            CLOSE_SOCKET(client_socket);
            std::cerr << "Registration failed for user: " << username << std::endl;
            return;
        }
    }

    // Authenticate user
    if (!user_manager_.authenticateUser(username, password)) {
        std::string auth_failed_msg = COLOR_RED "[Server]: Authentication failed. Invalid username or password." COLOR_RESET "\n";
        send(client_socket, auth_failed_msg.c_str(), static_cast<int>(auth_failed_msg.length()), 0);
        CLOSE_SOCKET(client_socket);
        std::cerr << "Authentication failed for user: " << username << std::endl;
        return;
    }

    std::cout << "User " << username << " authenticated successfully." << std::endl;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_[client_socket] = username; // Store client with socket as key and username as value
    }

    std::string welcome = COLOR_GREEN "[Server]: " + username + " has joined the chat!" COLOR_RESET "\n";
    broadcast(welcome, client_socket);
    std::cout << welcome;

    // Use the remaining data as leftover for chat messages
    std::string leftover = received_data;

    while (true)
    {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            break;
        }
        buffer[bytes_received] = '\0';
        leftover += buffer;

        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos)
        {
            std::string msg = leftover.substr(0, pos);
            leftover.erase(0, pos + 1);

            std::string formatted = "[" + username + "]: " + msg + "\n";
            std::cout << formatted;
            broadcast(formatted, client_socket);
        }
    }
    remove_client(client_socket);
    CLOSE_SOCKET(client_socket);
}

void ChatServer::broadcast(const std::string &message, int sender_socket)
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto const& [client_socket, username] : clients_)
    {
        if (client_socket != sender_socket)
        {
            send(client_socket, message.c_str(), static_cast<int>(message.length()), 0);
        }
    }
}

void ChatServer::remove_client(int socket)
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(socket);
    if (it != clients_.end()) {
        std::string username = it->second;
        std::cout << username << " has disconnected." << std::endl;
        clients_.erase(it);

        std::string goodbye = COLOR_YELLOW "[Server]: " + username + " has left the chat." COLOR_RESET "\n";
        broadcast(goodbye, -1); // Broadcast to all remaining clients
    }
}
