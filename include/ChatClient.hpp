#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <string>
#include <thread>

// Implements a chat client for connecting to the chat server.
class ChatClient
{
public:
    // Constructor: Initializes the ChatClient with server details and user credentials.
    ChatClient(const std::string &server_ip, int port, const std::string &username, const std::string &password);
    // Destructor: Cleans up client resources.
    ~ChatClient();
    // Runs the chat client, connecting to the server and managing message sending/receiving.
    void run();

private:
    // Connects the client to the chat server.
    void connect_to_server();
    // Receives messages from the server and displays them.
    void receive_messages();
    // Sends messages typed by the user to the server.
    void send_messages();
    // Cleans up socket resources.
    void cleanup();

    // Server IP address.
    std::string server_ip_;
    // Server port number.
    int port_;
    // Client socket file descriptor.
    int sock_;
    // User's username.
    std::string username_;
    // User's password.
    std::string password_;
    // Thread for receiving messages.
    std::thread receiver_thread_;
    // Flag indicating if the client is connected to the server.
    bool connected_ = false;
};

#endif // CHAT_CLIENT_HPP
