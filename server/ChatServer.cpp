// ChatServer.cpp (Cross-platform)
#include "../include/ChatServer.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <cstring>

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

ChatServer::ChatServer(int port) : port_(port), server_fd_(-1) {}

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
    char name_buffer[100] = {0};
    int name_len = recv(client_socket, name_buffer, sizeof(name_buffer) - 1, 0);
    if (name_len <= 0)
    {
        CLOSE_SOCKET(client_socket);
        return;
    }

    std::string username(name_buffer);
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.emplace_back(username, client_socket);
    }

    std::string welcome = "[Server]: " + username + " has joined the chat!\n";
    broadcast(welcome, client_socket);
    std::cout << welcome;

    char buffer[1024];
    std::string leftover;

    while (true)
    {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
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

    std::string goodbye = "[Server]: " + username + " has left the chat.\n";
    broadcast(goodbye, -1);
    std::cout << goodbye;
}

void ChatServer::broadcast(const std::string &message, int sender_socket)
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto &[name, client_socket] : clients_)
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
    for (auto it = clients_.begin(); it != clients_.end(); ++it)
    {
        if (it->second == socket)
        {
            std::cout << it->first << " has disconnected." << std::endl;
            clients_.erase(it);
            break;
        }
    }
}
