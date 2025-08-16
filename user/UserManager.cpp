#include "../include/user/UserManager.hpp"
#include <fstream>
#include <filesystem> // Re-added for std::filesystem::exists and std::filesystem::file_size
#include "../include/nlohmann/json.hpp"
#include <iostream> // For std::cerr

using json = nlohmann::json;

// Constructor: Initializes UserManager and ensures the data file exists and is valid.
UserManager::UserManager(const std::string& filename) : dataFile(filename) {
    // Check if the data file exists and is not empty
    if (!std::filesystem::exists(dataFile) || std::filesystem::file_size(dataFile) == 0) {
        // If not, create it and initialize with an empty JSON object
        std::ofstream outFile(dataFile);
        if (outFile.is_open()) {
            outFile << "{}";
        }
    } else {
        // If file exists, validate its JSON content; overwrite if corrupted
        std::ifstream checkFile(dataFile);
        std::string content((std::istreambuf_iterator<char>(checkFile)),
                            std::istreambuf_iterator<char>());
        checkFile.close();
        try {
            json::parse(content); // Try to parse to validate
        } catch (const json::parse_error& e) {
            std::cerr << "Corrupted JSON file detected: " << dataFile << ". Overwriting with empty object. Error: " << e.what() << std::endl;
            std::ofstream outFile(dataFile);
            if (outFile.is_open()) {
                outFile << "{}";
            }
        }
    }
    loadFromFile(); // Load valid data from the file
}

// Loads user data from the JSON file into memory.
void UserManager::loadFromFile() {
    std::ifstream inFile(dataFile);
    if (!inFile.is_open()) return; // Exit if file cannot be opened

    std::string content((std::istreambuf_iterator<char>(inFile)),
                        std::istreambuf_iterator<char>());
    inFile.close(); // Ensure file is closed after reading

    if (content.empty()) {
        return; // Nothing to load if file is empty
    }

    json j;
    try {
        j = json::parse(content);
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in " << dataFile << ": " << e.what() << std::endl;
        return; // Exit if JSON is invalid
    }
    // Populate users map from parsed JSON data
    for (const auto& [username, data] : j.items()) {
        User user(username, ""); // Initialize user
        user.passwordHash = data["passwordHash"].get<std::string>();
        user.friends = data["friends"].get<std::unordered_set<std::string>>();
        user.incomingRequests = data["incomingRequests"].get<std::unordered_set<std::string>>();
        user.outgoingRequests = data["outgoingRequests"].get<std::unordered_set<std::string>>();

        // Load chat history for the user
        for (const auto& [friendName, messages] : data["chatHistory"].items()) {
            for (const auto& msg : messages) {
                std::string sender = msg["sender"];
                std::string content = msg["content"];
                user.storeMessage(friendName, sender, content);
            }
        }
        users.emplace(username, user); // Add user to the map
    }
}

// Saves the current state of user data to the JSON file.
void UserManager::saveToFile() const {
    json j; // Create JSON object for saving
    // Populate JSON object from users map
    for (const auto& [username, user] : users) {
        json userJson;
        userJson["passwordHash"] = user.passwordHash;
        userJson["friends"] = user.friends;
        userJson["incomingRequests"] = user.incomingRequests;
        userJson["outgoingRequests"] = user.outgoingRequests;

        // Save chat history
        for (const auto& [friendName, messages] : user.chatHistory) {
            for (const auto& msg : messages) {
                userJson["chatHistory"][friendName].push_back({
                    {"sender", msg.sender},
                    {"content", msg.content}
                });
            }
        }

        j[username] = userJson; // Add user's data to the main JSON object
    }

    std::ofstream outFile(dataFile); // Open file for writing
    outFile << j.dump(4); // Write formatted JSON to file
}

// Checks if a user exists in the system.
bool UserManager::userExists(const std::string& username) const {
    return users.count(username) > 0; // Use count for existence check
}

// Registers a new user if the username is not already taken.
bool UserManager::registerUser(const std::string& username, const std::string& password) {
    if (userExists(username)) return false;
    users.emplace(username, User(username, password)); // Add new user
    saveToFile(); // Persist changes
    return true;
}

// Authenticates a user by checking their username and password.
bool UserManager::authenticateUser(const std::string& username, const std::string& password) const {
    auto it = users.find(username);
    return it != users.end() && it->second.checkPassword(password); // Check existence and password
}

// Retrieves a mutable User object by username.
std::optional<std::reference_wrapper<User>> UserManager::getUser(const std::string& username) {
    auto it = users.find(username);
    if (it == users.end()) return std::nullopt; // Return nullopt if user not found
    return it->second; // Return a reference to the user
}

// Retrieves a const User object by username.
std::optional<std::reference_wrapper<const User>> UserManager::getUser(const std::string& username) const {
    auto it = users.find(username);
    if (it == users.end()) return std::nullopt; // Return nullopt if user not found
    return it->second; // Return a const reference to the user
}

// Sends a friend request from one user to another.
bool UserManager::sendFriendRequest(const std::string& from, const std::string& to) {
    if (!userExists(from) || !userExists(to) || from == to) return false; // Validate users and prevent self-request

    User& sender = users.at(from);
    User& receiver = users.at(to);

    // Prevent duplicate or already accepted requests
    if (sender.hasSentRequestTo(to) || receiver.hasPendingRequestFrom(from) || sender.hasFriend(to)) return false;

    sender.sendFriendRequestTo(to); // Update sender's outgoing requests
    receiver.receiveFriendRequestFrom(from); // Update receiver's incoming requests
    saveToFile(); // Persist changes
    return true;
}

// Accepts a friend request.
bool UserManager::acceptFriendRequest(const std::string& username, const std::string& from) {
    if (!userExists(username) || !userExists(from)) return false; // Validate users

    User& receiver = users.at(username);
    User& sender = users.at(from);

    if (!receiver.hasPendingRequestFrom(from)) return false; // No pending request to accept

    bool success = receiver.acceptFriendRequestFrom(from); // Receiver accepts
    if (success) {
        sender.completeOutgoingFriendRequest(username); // Sender completes outgoing request
    }

    saveToFile(); // Persist changes
    return success; // Return true only if receiver successfully accepted
}

// Rejects a friend request from a sender.
bool UserManager::rejectFriendRequest(const std::string& rejecting_username, const std::string& sender_username) {
    if (!userExists(rejecting_username) || !userExists(sender_username)) return false; // Validate users

    User& rejector = users.at(rejecting_username);
    User& sender = users.at(sender_username);

    if (!rejector.hasPendingRequestFrom(sender_username)) return false; // No pending request to reject

    rejector.rejectFriendRequestFrom(sender_username); // Reject request for rejector
    sender.cancelOutgoingFriendRequest(rejecting_username); // Cancel outgoing request for sender
    saveToFile(); // Persist changes
    return true;
}

// Retrieves a user's incoming friend requests.
std::optional<std::reference_wrapper<const std::unordered_set<std::string>>> UserManager::getIncomingFriendRequests(const std::string& username) const {
    auto it = users.find(username);
    if (it == users.end()) {
        return std::nullopt; // Return nullopt if user not found
    }
    return it->second.getIncomingFriendRequests(); // Return const reference to incoming requests
}

// Stores a chat message between two users.
void UserManager::storeMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    if (!userExists(sender) || !userExists(receiver)) return; // Validate users

    // Store message for both sender and receiver
    users.at(sender).storeMessage(receiver, sender, content);
    users.at(receiver).storeMessage(sender, sender, content);
    saveToFile(); // Persist changes
}
