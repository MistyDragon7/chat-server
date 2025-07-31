#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <linux/close_range.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

#define PORT 9000

void handle_client(int client_socket)
{
    FILE *stream = fdopen(client_socket, "r+");
    if (!stream)
    {
        perror("fdopen failed");
        close(client_socket);
        return;
    }

    char *line = nullptr;
    size_t len = 0;

    while (getline(&line, &len, stream) != -1)
    {
        std::cout << "Received: " << line;
        // Echo the same message back to client
        fprintf(stream, "%s", line);
        fflush(stream);
    }

    std::cout << "Client Disconnected Or Error Occured.\n";
    free(line);
    fclose(stream); // Also closes client_socket
}
int main()
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    // 1. Create socket
    // Everywhere, fd shall stand for file descriptor.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
               sizeof(opt));
    // Bind to Port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
    {
        perror("Bind Failed!");
        exit(EXIT_FAILURE);
    }
    // Listening To the Port
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen Failed!");
        exit(EXIT_FAILURE);
    }
    std::cout << "Now Listening on Port: " << PORT << std::endl;
    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            perror("accept");
            return 1;
        }
        // Receiving message
        std::thread t(handle_client, client_socket);
        t.detach(); // Let the thread run independently
    }
    close(server_fd);
    return 0;
}
