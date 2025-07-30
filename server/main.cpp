#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <linux/close_range.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9000

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    // 1. Create socket
    // Everywhere, fd shall stand for file descriptor.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd==0) {
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    // Bind to Port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, addrlen)<0) {
        perror("Bind Failed!");
        exit(EXIT_FAILURE);
    }
    // Listening To the Port
    if (listen(server_fd, 3)<0) {
        perror("Listen Failed!");
        exit(EXIT_FAILURE);
    }
    std::cout<<"Now Listening on Port: "<<PORT<<std::endl;
    client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_socket<0) {
        perror("accept");
        return 1;
    }
    //Receiving message
    char buffer[1024] = {0};
    int readval = recv(client_socket, buffer, 1024, 0);
    std::cout<<"Client Says: "<<buffer<<std::endl;
    const char* response = "Hello Client! \n";
    send(client_socket, response, strlen(response), 0);
    // Cleanup
    close(client_socket);
    close(server_fd);
    return 0;
}
