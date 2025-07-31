// chatClient.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>

#define PORT 9000

int ChatClient()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. Convert address
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address / Address not supported\n";
        return 1;
    }

    // 3. Connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        return 1;
    }
    std::cout << "Connected to server. Type messages to send. Type 'exit' to quit.\n";
    while (true)
    {
        std::string input;
        std::getline(std::cin, input);
        if (input == "exit")
            break;
        send(sock, input.c_str(), input.size(), 0);
        send(sock, "\n", 1, 0);
        char buffer[1024] = {0};
        int valread = recv(sock, buffer, sizeof(buffer), 0);
        if (valread <= 0)
        {
            std::cerr << "Server disconnected or error: \n";
            break;
        }
        std::cout << "Server says: " << buffer << std::endl;
    }
    close(sock);
    return 0;
}
