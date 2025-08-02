#include "../include/ChatClient.hpp"
#include "../include/Color.hpp"
#include <iostream>
#include <thread>
#include <string>
#include "../include/Common.hpp" // Include Common.hpp
// #include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#endif

#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

ChatClient::ChatClient(const std::string &server_ip, int port, const std::string &username, const std::string &password)
    : server_ip_(server_ip), port_(port), sock_(-1), username_(username), password_(password), connected_(false) {}

ChatClient::~ChatClient()
{
    cleanup();
}

void ChatClient::connect_to_server()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << COLOR_RED << "WSAStartup failed" << COLOR_RESET << std::endl;
        exit(1);
    }
#endif

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
    // Send handshake magic string immediately after connecting
    send(sock_, CLIENT_HANDSHAKE_MAGIC.c_str(), static_cast<int>(CLIENT_HANDSHAKE_MAGIC.length()), 0);

    // Send username and password to server, each terminated by a newline
    std::string username_with_newline = username_ + "\n";
    send(sock_, username_with_newline.c_str(), static_cast<int>(username_with_newline.length()), 0);

    std::string password_with_newline = password_ + "\n";
    send(sock_, password_with_newline.c_str(), static_cast<int>(password_with_newline.length()), 0);
}

void ChatClient::receive_messages()
{
    char buffer[1024];
    while (connected_)
    {
        int bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);
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
    std::string message;
    std::cout << COLOR_CYAN << "> " << COLOR_RESET;
    while (std::getline(std::cin, message))
    {
        if (message == "/quit" || std::cin.eof())
            break;

        message += "\n";
        send(sock_, message.c_str(), static_cast<int>(message.length()), 0);

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
        CLOSE_SOCKET(sock_);
        sock_ = -1;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}
