#include "../include/ChatClient.hpp"
#include <iostream>
#include <string>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

std::string getLocalIPAddress()
{
    struct ifaddrs *ifaddr, *ifa;
    std::string ipAddress = "Unavailable";

    if (getifaddrs(&ifaddr) == -1)
    {
        return ipAddress;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET &&
            !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            char addr[INET_ADDRSTRLEN];
            void *sa = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, sa, addr, INET_ADDRSTRLEN);
            ipAddress = addr;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ipAddress;
}

int main()
{
    std::string localIP = getLocalIPAddress();
    std::cout << "Your local IP address is: " << localIP << std::endl;

    std::string serverIP;
    std::cout << "Enter the server IP to connect to: ";
    std::getline(std::cin, serverIP);

    int port = 9000;
    ChatClient client(serverIP, port);
    client.run();

    return 0;
}
