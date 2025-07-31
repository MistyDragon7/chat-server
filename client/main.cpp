#include "ChatClient.hpp"

int main()
{
    ChatClient client("127.0.0.1", 9000);
    client.run();
    return 0;
}
