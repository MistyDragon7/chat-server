#include "../include/user/UserManager.hpp"
#include <fstream>
#include <filesystem>
#include "../include/nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

// Initializes UserManager, ensuring the user data file exists and is valid, then loads data.
UserManager::UserManager(const std::string& filename) : dataFile(filename) {
    if (!std::filesystem::exists(dataFile) || std::filesystem::file_size(dataFile) == 0) {
        std::ofstream outFile(dataFile);
        if (outFile.is_open()) {
            outFile << "{}";
        }
    } else {
        std::ifstream checkFile(dataFile);
        std::string content((std::istreambuf_iterator<char>(checkFile)),
                            std::istreambuf_iterator<char>());
        checkFile.close();
        try {
            nlohmann::json::parse(content);
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Corrupted JSON file detected: " << dataFile << ". Overwriting with empty object. Error: " << e.what() << std::endl;
            std::ofstream outFile(dataFile);
            if (outFile.is_open()) {
                outFile << "{}";
            }
        }
    }
    loadFromFile();
}

// Loads user data from the JSON file into memory.
void UserManager::loadFromFile() {
    std::ifstream inFile(dataFile);
    if (!inFile.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(inFile)),
                        std::istreambuf_iterator<char>());
    inFile.close();

    if (content.empty()) {
        return;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(content);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error in " << dataFile << ": " << e.what() << std::endl;
        return;
    }

    // Populate users map from parsed JSON data.
    for (const auto& [username, data] : j.items()) {
        User user(username, "");
        user.passwordHash = data["passwordHash"].get<std::string>();
        user.friends = data["friends"].get<std::unordered_set<std::string>>();
        user.incomingRequests = data["incomingRequests"].get<std::unordered_set<std::string>>();
        user.outgoingRequests = data["outgoingRequests"].get<std::unordered_set<std::string>>();

        // Load chat history for the user.
        for (const auto& [friendName, messages] : data["chatHistory"].items()) {
            for (const auto& msg : messages) {
                std::string sender = msg["sender"];
                std::string content = msg["content"];
                user.storeMessage(friendName, sender, content);
            }
        }
        users.emplace(username, user);
    }
}

// Saves the current state of user data to the JSON file.
void UserManager::saveToFile() const {
    nlohmann::json j;

    // Populate JSON object from users map.
    for (const auto& [username, user] : users) {
        nlohmann::json userJson;
        userJson["passwordHash"] = user.passwordHash;
        userJson["friends"] = user.friends;
        userJson["incomingRequests"] = user.incomingRequests;
        userJson["outgoingRequests"] = user.outgoingRequests;

        // Save chat history.
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

// Checks if a user exists in the system.
bool UserManager::userExists(const std::string& username) const {
    return users.count(username) > 0;
}

// Registers a new user if the username is not already taken and persists changes.
bool UserManager::registerUser(const std::string& username, const std::string& password) {
    if (userExists(username)) return false;
    users.emplace(username, User(username, password));
    saveToFile();
    return true;
}

// Authenticates a user by checking their username and password.
bool UserManager::authenticateUser(const std::string& username, const std::string& password) const {
    auto it = users.find(username);
    return it != users.end() && it->second.checkPassword(password);
}

// Retrieves a mutable User object by username, if found.
std::optional<std::reference_wrapper<User>> UserManager::getUser(const std::string& username) {
    auto it = users.find(username);
    if (it == users.end()) return std::nullopt;
    return it->second;
}

// Retrieves a const User object by username, if found.
std::optional<std::reference_wrapper<const User>> UserManager::getUser(const std::string& username) const {
    auto it = users.find(username);
    if (it == users.end()) return std::nullopt;
    return it->second;
}

// Sends a friend request from one user to another, with validation and persistence.
bool UserManager::sendFriendRequest(const std::string& from, const std::string& to) {
    if (!userExists(from) || !userExists(to) || from == to) return false;

    User& sender = users.at(from);
    User& receiver = users.at(to);

    // Prevent duplicate or already accepted requests.
    if (sender.hasSentRequestTo(to) || receiver.hasPendingRequestFrom(from) || sender.hasFriend(to)) return false;

    sender.sendFriendRequestTo(to);
    receiver.receiveFriendRequestFrom(from);
    saveToFile();
    return true;
}

// Accepts a friend request, updating both users' states and persisting changes.
bool UserManager::acceptFriendRequest(const std::string& username, const std::string& from) {
    if (!userExists(username) || !userExists(from)) return false;

    User& receiver = users.at(username);
    User& sender = users.at(from);

    if (!receiver.hasPendingRequestFrom(from)) return false;

    bool success = receiver.acceptFriendRequestFrom(from);
    if (success) {
        sender.completeOutgoingFriendRequest(username);
    }

    saveToFile();
    return success;
}

// Rejects a friend request, updating both users' states and persisting changes.
bool UserManager::rejectFriendRequest(const std::string& rejecting_username, const std::string& sender_username) {
    if (!userExists(rejecting_username) || !userExists(sender_username)) return false;

    User& rejector = users.at(rejecting_username);
    User& sender = users.at(sender_username);

    if (!rejector.hasPendingRequestFrom(sender_username)) return false;

    rejector.rejectFriendRequestFrom(sender_username);
    sender.cancelOutgoingFriendRequest(rejecting_username);
    saveToFile();
    return true;
}

// Retrieves a user's incoming friend requests, if the user exists.
std::optional<std::reference_wrapper<const std::unordered_set<std::string>>> UserManager::getIncomingFriendRequests(const std::string& username) const {
    auto it = users.find(username);
    if (it == users.end()) {
        return std::nullopt;
    }
    return it->second.getIncomingFriendRequests();
}

// Stores a chat message between two users and persists changes.
void UserManager::storeMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    if (!userExists(sender) || !userExists(receiver)) return;

    users.at(sender).storeMessage(receiver, sender, content);
    users.at(receiver).storeMessage(sender, sender, content);
    saveToFile();
}
