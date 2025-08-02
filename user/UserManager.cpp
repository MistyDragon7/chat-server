#include "../include/user/UserManager.hpp"
#include <fstream>
#include "../include/nlohmann/json.hpp"
#include <iostream> // For std::cerr

using json = nlohmann::json;

UserManager::UserManager(const std::string& filename) : dataFile(filename) {
    loadFromFile();
}

void UserManager::loadFromFile() {
    std::ifstream inFile(dataFile);
    if (!inFile.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(inFile)),
                        std::istreambuf_iterator<char>());
    inFile.close(); // Close the file stream after reading content

    if (content.empty()) {
        // File is empty, nothing to load
        return;
    }

    json j;
    try {
        j = json::parse(content);
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in " << dataFile << ": " << e.what() << std::endl;
        return;
    }
    for (const auto& [username, data] : j.items()) {
        User user(username, "");  // Password will be set directly from hash
        user.passwordHash = data["passwordHash"].get<std::string>();
        user.friends = data["friends"].get<std::unordered_set<std::string>>();
        user.incomingRequests = data["incomingRequests"].get<std::unordered_set<std::string>>();
        user.outgoingRequests = data["outgoingRequests"].get<std::unordered_set<std::string>>();

        for (const auto& [friendName, messages] : data["chatHistory"].items()) {
            for (const auto& msg : messages) {
                std::string sender = msg["sender"];
                std::string content = msg["content"];
                user.storeMessage(friendName, sender, content);
            }
        }
        users.emplace(username, user); // Use emplace instead of operator[]
    }
}

void UserManager::saveToFile() const {
    json j;
    for (const auto& [username, user] : users) {
        json userJson;
        userJson["passwordHash"] = user.passwordHash;
        userJson["friends"] = user.friends;
        userJson["incomingRequests"] = user.incomingRequests;
        userJson["outgoingRequests"] = user.outgoingRequests;

        for (const auto& [friendName, messages] : user.chatHistory) {
            for (const auto& msg : messages) {
                userJson["chatHistory"][friendName].push_back({
                    {"sender", msg.sender},
                    {"content", msg.content}
                });
            }
        }

        j[username] = userJson;
    }

    std::ofstream outFile(dataFile);
    outFile << j.dump(4);
}

bool UserManager::userExists(const std::string& username) const {
    return users.find(username) != users.end();
}

bool UserManager::registerUser(const std::string& username, const std::string& password) {
    if (userExists(username)) return false;
    users.emplace(username, User(username, password));
    saveToFile();
    return true;
}

bool UserManager::authenticateUser(const std::string& username, const std::string& password) const {
    auto it = users.find(username);
    return it != users.end() && it->second.checkPassword(password);
}

std::optional<std::reference_wrapper<User>> UserManager::getUser(const std::string& username) {
    auto it = users.find(username);
    if (it == users.end()) return std::nullopt;
    return it->second;
}

std::optional<std::reference_wrapper<const User>> UserManager::getUser(const std::string& username) const {
    auto it = users.find(username);
    if (it == users.end()) return std::nullopt;
    return it->second;
}

bool UserManager::sendFriendRequest(const std::string& from, const std::string& to) {
    if (!userExists(from) || !userExists(to) || from == to) return false;

    User& sender = users.at(from); // Use .at() instead of operator[]
    User& receiver = users.at(to);   // Use .at() instead of operator[]

    if (sender.hasSentRequestTo(to) || receiver.hasPendingRequestFrom(from)) return false;

    sender.sendFriendRequestTo(to);
    receiver.receiveFriendRequestFrom(from);
    saveToFile();
    return true;
}

bool UserManager::acceptFriendRequest(const std::string& username, const std::string& from) {
    if (!userExists(username) || !userExists(from)) return false;

    User& receiver = users.at(username); // Use .at() instead of operator[]
    User& sender = users.at(from);       // Use .at() instead of operator[]

    if (!receiver.hasPendingRequestFrom(from)) return false;

    bool success1 = receiver.acceptFriendRequestFrom(from);
    bool success2 = sender.acceptFriendRequestFrom(username);

    saveToFile();
    return success1 && success2;
}

void UserManager::storeMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    if (!userExists(sender) || !userExists(receiver)) return;

    users.at(sender).storeMessage(receiver, sender, content); // Use .at() instead of operator[]
    users.at(receiver).storeMessage(sender, sender, content); // Use .at() instead of operator[]
    saveToFile();
}
