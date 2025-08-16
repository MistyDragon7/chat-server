// include/ChatServer.hpp

#ifndef CHAT_SERVER_HPP
#define CHAT_SERVER_HPP

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define closesocket close
#endif

#include <string>
#include <mutex>
#include <map> // For std::map

#include "user/UserManager.hpp" // Include UserManager
#include "Common.hpp" // Re-added Common.hpp for CLIENT_HANDSHAKE_MAGIC

class ChatServer
{
public:
    // Constructor: Initializes the ChatServer with the specified port.
    ChatServer(int port);
    // Destructor: Cleans up resources when the ChatServer is destroyed.
    ~ChatServer();
    // Starts the server, making it listen for incoming client connections.
    void start();

private:
    // Reads a newline-delimited message from a client socket.
    std::optional<std::string> read_delimited_message(int client_socket, std::string& leftover_buffer);
    // Processes chat commands (e.g., /friend, /msg, /quit, /pending) sent by clients.
    void process_chat_command(int client_socket, const std::string& sender_username, const std::string& message);
    // Accepts incoming client connections in a loop.
    void accept_clients();
    // Handles a single client connection, including authentication and message processing.
    void handle_client(int client_socket);
    // Broadcasts a message to all connected clients except the sender.
    void broadcast(const std::string &message, int sender_socket);
    // Removes a disconnected client from the server's active client list.
    void remove_client(int socket);
    // Disconnects a client, broadcasts a departure message, and cleans up socket resources.
    void disconnect_client(int client_socket, const std::string& username);

    // Server port number.
    int port_;
    // Server socket file descriptor.
    int server_fd_;
    // Map to store active clients, associating socket with username.
    std::map<int, std::string> clients_;
    // Mutex to protect access to the clients_ map.
    std::mutex clients_mutex_;
    // Flag indicating if the server is running.
    bool running_ = false;
    // Manages user authentication, registration, and friend requests.
    UserManager user_manager_;
};

#endif // CHAT_SERVER_HPP