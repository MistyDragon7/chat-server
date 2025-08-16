// ChatServer.cpp (Cross-platform)
#include "../include/ChatServer.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <map>
#include <optional> // Re-added optional
// Removed optional since it's now in ChatServer.hpp

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

#include "../include/user/UserManager.hpp"
#include "../include/Color.hpp"
// Removed Common.hpp since it's no longer needed in ChatServer.cpp

// Removed global UserManager as it's now a member of ChatServer

// Constructor: Initializes ChatServer with a given port and sets up UserManager.
ChatServer::ChatServer(int port) : port_(port), server_fd_(-1), user_manager_("users.json") {}

// Destructor: Cleans up socket resources when ChatServer is destroyed.
ChatServer::~ChatServer()
{
    if (server_fd_ != -1)
    {
        CLOSE_SOCKET(server_fd_);
    }
#ifdef _WIN32
    WSACleanup(); // Cleans up WinSock resources
#endif
}

// Starts the chat server, initializing networking and listening for connections.
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

    // Create socket file descriptor
    server_fd_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (server_fd_ < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

// Set socket options for reuse of address and port
#ifndef _WIN32
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
#else
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#endif

    // Bind socket to the specified port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd_, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    running_ = true;
    std::cout << "Server listening on port: " << port_ << std::endl;
    accept_clients();
}

// Continuously accepts new client connections.
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
        std::thread(&ChatServer::handle_client, this, client_socket).detach(); // Detach thread for each client
    }
}

/*
* Handles individual client connections.
* Authenticates users, processes chat messages and commands.
*/
void ChatServer::handle_client(int client_socket)
{
    std::string received_data_leftover; // Leftover data after initial reads

    // Handshake, username, password reception/validation
    auto read_and_validate = [&](const std::string& type) -> std::optional<std::string> {
        std::optional<std::string> data_opt = read_delimited_message(client_socket, received_data_leftover);
        if (!data_opt) {
            std::cerr << "Client disconnected during " << type << " reception or sent no data." << std::endl;
            disconnect_client(client_socket, ""); // Username not yet known for initial handshakes
            return std::nullopt;
        }
        return data_opt;
    };

    std::optional<std::string> handshake_opt = read_and_validate("handshake");
    if (!handshake_opt) return;
    std::string handshake_received = *handshake_opt;

    if (handshake_received != CLIENT_HANDSHAKE_MAGIC.substr(0, CLIENT_HANDSHAKE_MAGIC.length() -1)) {
        std::cerr << COLOR_RED << "Invalid handshake from client: '" << handshake_received << "'" << COLOR_RESET << std::endl;
        CLOSE_SOCKET(client_socket);
        return;
    }

    std::optional<std::string> username_opt = read_and_validate("username");
    if (!username_opt) return;
    std::string username = *username_opt;

    std::optional<std::string> password_opt = read_and_validate("password");
    if (!password_opt) return;
    std::string password = *password_opt;

    // User registration/authentication
    if (!user_manager_.userExists(username)) {
        if (user_manager_.registerUser(username, password)) {
            std::cout << "New user " << username << " registered successfully." << std::endl;
        } else {
            std::string reg_failed_msg = COLOR_RED "[Server]: Registration failed for user: " + username + ". Please try again." COLOR_RESET "\n";
            send(client_socket, reg_failed_msg.c_str(), static_cast<int>(reg_failed_msg.length()), 0);
            disconnect_client(client_socket, username);
            std::cerr << "Registration failed for user: " << username << std::endl;
            return;
        }
    }

    if (!user_manager_.authenticateUser(username, password)) {
        std::string auth_failed_msg = COLOR_RED "[Server]: Authentication failed. Invalid username or password." COLOR_RESET "\n";
        send(client_socket, auth_failed_msg.c_str(), static_cast<int>(auth_failed_msg.length()), 0);
        disconnect_client(client_socket, username);
        std::cerr << "Authentication failed for user: " << username << std::endl;
        return;
    }

    std::cout << "User " << username << " authenticated successfully." << std::endl;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_[client_socket] = username; // Store client with socket as key and username as value
    }

    std::string welcome = COLOR_GREEN "[Server]: " + username + " has joined the chat!" COLOR_RESET "\n";
    broadcast(welcome, client_socket);
    std::cout << welcome;

    std::string leftover = received_data_leftover;

    // Main chat loop: Continuously read and process messages from the client.
    while (true) {
        std::optional<std::string> msg_opt = read_delimited_message(client_socket, leftover);
        if (!msg_opt) {
            break;
        }
        std::string msg = *msg_opt;

        if (msg.rfind("/", 0) == 0) {
            process_chat_command(client_socket, username, msg);
        } else {
            std::string formatted = "[" + username + "]: " + msg + "\n";
            std::cout << formatted;
            broadcast(formatted, client_socket);
        }
    }
    // Client disconnected, clean up resources.
    disconnect_client(client_socket, username);
}

// Helper function to read a newline-delimited message from a socket.
std::optional<std::string> ChatServer::read_delimited_message(int client_socket, std::string& leftover_buffer) {
    char temp_buffer[1024];
    while (true) {
        size_t newline_pos = leftover_buffer.find('\n');
        if (newline_pos != std::string::npos) {
            std::string message = leftover_buffer.substr(0, newline_pos);
            leftover_buffer.erase(0, newline_pos + 1);
            // Strip carriage return if present for cross-platform compatibility
            if (!message.empty() && message.back() == '\r') {
                message.pop_back();
            }
            return message;
        }

        int bytes_received = recv(client_socket, temp_buffer, sizeof(temp_buffer) - 1, 0);
        if (bytes_received <= 0) {
            // Client disconnected or error during read
            return std::nullopt;
        }
        temp_buffer[bytes_received] = '\0'; // Null-terminate received data
        leftover_buffer += temp_buffer;
    }
}

// Handles various chat commands received from clients (e.g., /friend, /msg, /quit, /pending).
void ChatServer::process_chat_command(int client_socket, const std::string& sender_username, const std::string& message) {
    if (message.rfind("/friend ", 0) == 0) { // Check for /friend command
        std::string command_args = message.substr(8); // Extract arguments after "/friend "
        size_t space_pos = command_args.find(' ');
        if (space_pos == std::string::npos) {
            std::string error_msg = COLOR_RED "[Server]: Invalid friend command format. Use /friend add <username>, /friend accept <username>, or /friend reject <username>." COLOR_RESET "\n";
            send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            return;
        }
        std::string sub_command = command_args.substr(0, space_pos);
        std::string target_username = command_args.substr(space_pos + 1);

        if (sub_command == "add") { // Handle friend request addition
            if (user_manager_.sendFriendRequest(sender_username, target_username)) {
                std::string success_msg = COLOR_GREEN "[Server]: Friend request sent to " + target_username + "." COLOR_RESET "\n";
                send(client_socket, success_msg.c_str(), static_cast<int>(success_msg.length()), 0);
                // Notify target user if online about incoming friend request
                for (auto const& [sock, uname] : clients_) {
                    if (uname == target_username) {
                        std::string notification = COLOR_YELLOW "[Server]: " + sender_username + " has sent you a friend request! Use /friend accept " + sender_username + " to accept." COLOR_RESET "\n";
                        send(sock, notification.c_str(), static_cast<int>(notification.length()), 0);
                        break;
                    }
                }
            } else {
                std::string error_msg = COLOR_RED "[Server]: Failed to send friend request to " + target_username + ". (User not found, already friends, or request pending)" COLOR_RESET "\n";
                send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            }
        } else if (sub_command == "accept") { // Handle friend request acceptance
            if (user_manager_.acceptFriendRequest(sender_username, target_username)) {
                std::string success_msg = COLOR_GREEN "[Server]: You are now friends with " + target_username + "." COLOR_RESET "\n";
                send(client_socket, success_msg.c_str(), static_cast<int>(success_msg.length()), 0);
                // Notify target user if online about accepted friend request
                for (auto const& [sock, uname] : clients_) {
                    if (uname == target_username) {
                        std::string notification = COLOR_GREEN "[Server]: " + sender_username + " has accepted your friend request!" COLOR_RESET "\n";
                        send(sock, notification.c_str(), static_cast<int>(notification.length()), 0);
                        break;
                    }
                }
            } else {
                std::string error_msg = COLOR_RED "[Server]: Failed to accept friend request from " + target_username + ". (No pending request or user not found)" COLOR_RESET "\n";
                send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            }
        } else if (sub_command == "reject") { // Handle friend request rejection
            if (user_manager_.rejectFriendRequest(sender_username, target_username)) {
                std::string success_msg = COLOR_GREEN "[Server]: Friend request from " + target_username + " rejected." COLOR_RESET "\n";
                send(client_socket, success_msg.c_str(), static_cast<int>(success_msg.length()), 0);
            } else {
                std::string error_msg = COLOR_RED "[Server]: Failed to reject friend request from " + target_username + ". (No pending request or user not found)" COLOR_RESET "\n";
                send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            }
        } else {
            std::string error_msg = COLOR_RED "[Server]: Unknown friend command: " + sub_command + ". Use add, accept, or reject." COLOR_RESET "\n";
            send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
        }
    } else if (message.rfind("/msg ", 0) == 0) { // Check for /msg command for direct messaging
        std::string command_args = message.substr(5); // Extract arguments after "/msg "
        size_t first_space = command_args.find(' ');
        if (first_space == std::string::npos) {
            std::string error_msg = COLOR_RED "[Server]: Invalid message format. Use /msg <username> <message>." COLOR_RESET "\n";
            send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            return;
        }
        std::string recipient_username = command_args.substr(0, first_space);
        std::string dm_content = command_args.substr(first_space + 1);

        std::optional<std::reference_wrapper<User>> sender_user_opt = user_manager_.getUser(sender_username);
        std::optional<std::reference_wrapper<User>> recipient_user_opt = user_manager_.getUser(recipient_username);

        if (!sender_user_opt || !recipient_user_opt) {
            std::string error_msg = COLOR_RED "[Server]: User not found." COLOR_RESET "\n";
            send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            return;
        }

        User& sender_user = sender_user_opt->get();
        User& recipient_user = recipient_user_opt->get();

        if (!sender_user.hasFriend(recipient_username)) {
            std::string error_msg = COLOR_RED "[Server]: You are not friends with " + recipient_username + "." COLOR_RESET "\n";
            send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.length()), 0);
            return;
        }

        user_manager_.storeMessage(sender_username, recipient_username, dm_content);
        std::string formatted_dm = COLOR_MAGENTA "[DM from " + sender_username + "]: " + dm_content + COLOR_RESET + "\n";

        // Send DM to recipient if online, otherwise store message
        bool recipient_online = false;
        for (auto const& [sock, uname] : clients_) {
            if (uname == recipient_username) {
                send(sock, formatted_dm.c_str(), static_cast<int>(formatted_dm.length()), 0);
                recipient_online = true;
                break;
            }
        }

        if (recipient_online) {
            std::string success_msg = COLOR_GREEN "[Server]: Message sent to " + recipient_username + "." COLOR_RESET "\n";
            send(client_socket, success_msg.c_str(), static_cast<int>(success_msg.length()), 0);
        } else {
            std::string info_msg = COLOR_YELLOW "[Server]: " + recipient_username + " is offline. Message stored." COLOR_RESET "\n";
            send(client_socket, info_msg.c_str(), static_cast<int>(info_msg.length()), 0);
        }

    } else if (message == "/quit") { // Handle /quit command to disconnect client
        std::string goodbye_msg = COLOR_YELLOW "[Server]: You have successfully disconnected." COLOR_RESET "\n";
        send(client_socket, goodbye_msg.c_str(), static_cast<int>(goodbye_msg.length()), 0);
        disconnect_client(client_socket, sender_username);
        return; // Exit thread for this client

    } else if (message == "/pending") { // Handle /pending command to list friend requests
        std::optional<std::reference_wrapper<const std::unordered_set<std::string>>> pending_requests_opt = user_manager_.getIncomingFriendRequests(sender_username);
        if (pending_requests_opt && !pending_requests_opt->get().empty()) {
            std::string response = COLOR_CYAN "[Server]: Pending friend requests:\n" COLOR_RESET;
            for (const std::string& req_sender : pending_requests_opt->get()) {
                response += COLOR_CYAN "- " + req_sender + "\n" COLOR_RESET;
            }
            send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
        } else {
            std::string no_requests_msg = COLOR_CYAN "[Server]: No pending friend requests." COLOR_RESET "\n";
            send(client_socket, no_requests_msg.c_str(), static_cast<int>(no_requests_msg.length()), 0);
        }

    } else { // If not a command, treat as a public chat message
        std::string formatted = "[" + sender_username + "]: " + message + "\n";
        std::cout << formatted;
        broadcast(formatted, client_socket);
    }
}

// Broadcasts a message to all connected clients except the sender.
void ChatServer::broadcast(const std::string &message, int sender_socket)
{
    std::lock_guard<std::mutex> lock(clients_mutex_); // Protects access to clients_ map
    for (auto const& [client_socket, username] : clients_)
    {
        if (client_socket != sender_socket)
        {
            send(client_socket, message.c_str(), static_cast<int>(message.length()), 0);
        }
    }
}

// Removes a client from the active client list.
void ChatServer::remove_client(int socket)
{
    std::lock_guard<std::mutex> lock(clients_mutex_); // Protects access to clients_ map
    auto it = clients_.find(socket);
    if (it != clients_.end()) {
        std::string username = it->second;
        std::cout << username << " has disconnected." << std::endl;
        clients_.erase(it);
    }
}

// Disconnects a client, broadcasts a departure message, and cleans up resources.
void ChatServer::disconnect_client(int client_socket, const std::string& username) {
    std::string goodbye_message = COLOR_YELLOW "[Server]: " + username + " has left the chat." COLOR_RESET "\n";
    broadcast(goodbye_message, client_socket); // Broadcast before removing client
    remove_client(client_socket);
    // Shutdown and close the socket
    shutdown(client_socket, SD_SEND);
    CLOSE_SOCKET(client_socket);
}