#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. Convert address
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported\n";
        return 1;
    }

    // 3. Connect
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        return 1;
    }

    // 4. Send message
    const char* hello = "Hello, server!\n";
    send(sock, hello, strlen(hello), 0);

    // 5. Receive response
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    std::cout << "Server says: " << buffer << std::endl;

    close(sock);
    return 0;
}
