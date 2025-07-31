#include "ChatClient.hpp"
#include "Color.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

ChatClient::ChatClient(const std::string &server_ip, int port)
    : server_ip_(server_ip), port_(port), sock_(-1), connected_(false) {}

ChatClient::~ChatClient()
{
    cleanup();
}

void ChatClient::connect_to_server()
{
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, server_ip_.c_str(), &serv_addr.sin_addr) <= 0)
    {
        std::cerr << COLOR_RED << "Invalid address\n"
                  << COLOR_RESET;

        exit(1);
    }

    if (connect(sock_, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    connected_ = true;
}

void ChatClient::receive_messages()
{
    char buffer[1024];
    while (connected_)
    {
        ssize_t bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
        {
            std::cout << "\nDisconnected from server.\n";
            break;
        }
        buffer[bytes_received] = '\0';

        std::string message(buffer);
        if (!message.empty() && message.back() == '\n')
        {
            message.pop_back();
        }

        std::cout << COLOR_YELLOW << "\r" << message << COLOR_RESET << "\n"
                  << COLOR_CYAN << "> " << COLOR_RESET << std::flush;
    }

    connected_ = false;
}

void ChatClient::send_messages()
{
    std::string username;
    std::cout << COLOR_BLUE << "Enter your username: " << COLOR_RESET;
    std::getline(std::cin, username);
    send(sock_, username.c_str(), username.length(), 0);

    std::string message;
    std::cout << COLOR_CYAN << "> " << COLOR_RESET;
    while (std::getline(std::cin, message))
    {
        if (message == "/quit" || std::cin.eof())
            break;

        message += "\n";
        send(sock_, message.c_str(), message.length(), 0);

        std::cout << COLOR_CYAN << "> " << COLOR_RESET;
    }

    connected_ = false;
    std::cout << COLOR_MAGENTA << "[You have left the chat]\n"
              << COLOR_RESET;
}

void ChatClient::run()
{
    connect_to_server();
    receiver_thread_ = std::thread(&ChatClient::receive_messages, this);
    send_messages();
    receiver_thread_.detach();
    cleanup();
}

void ChatClient::cleanup()
{
    if (sock_ != -1)
    {
        close(sock_);
        sock_ = -1;
    }
}
