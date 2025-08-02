#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <string>
#include <thread>

class ChatClient
{
public:
    ChatClient(const std::string &server_ip, int port, const std::string &username, const std::string &password);
    ~ChatClient();

    void run();

private:
    void connect_to_server();
    void receive_messages();
    void send_messages();
    void cleanup();

    std::string server_ip_;
    int port_;
    int sock_;
    std::string username_;
    std::string password_;
    std::thread receiver_thread_;
    bool connected_;
};

#endif // CHAT_CLIENT_HPP
