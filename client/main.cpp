#include "../include/ChatClient.hpp"
#include <iostream>
#include <string>

// Main function to run the chat client application.
int main()
{
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
