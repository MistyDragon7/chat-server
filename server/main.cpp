#include "../include/ChatServer.hpp"

int main()
{
    ChatServer server(9000);
    server.start();
    return 0;
}
