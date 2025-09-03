#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>

// Magic string for client handshake to ensure proper protocol communication.
const std::string CLIENT_HANDSHAKE_MAGIC = "CHAT_HS_V1\n";

#endif // COMMON_HPP