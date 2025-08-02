#include "../include/ChatClient.hpp"
#include <iostream>
#include <string>

// No getLocalIPAddress function for cross-platform simplicity

int main()
{
    // Removed getLocalIPAddress() call
    // std::string localIP = getLocalIPAddress();
    // std::cout << "Your local IP address is: " << localIP << std::endl;

    std::string serverIP;
    std::cout << "Enter the server IP to connect to: ";
    std::getline(std::cin, serverIP);

    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    std::string password;
    std::cout << "Enter your password: ";
    std::getline(std::cin, password);

    int port = 9000;
    ChatClient client(serverIP, port, username, password);
    client.run();

    return 0;
}
