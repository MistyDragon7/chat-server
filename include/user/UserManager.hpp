#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include "User.hpp"
#include <unordered_map>
#include <string>
#include <optional>

// Manages user data, including registration, authentication, friend requests, and chat history.
class UserManager {
private:
    // Stores user data with username as key.
    std::unordered_map<std::string, User> users;
    // Path to the JSON file where user data is stored.
    std::string dataFile;

    // Loads user data from the JSON file.
    void loadFromFile();
    // Removed: // void saveToFile() const; // Moved to public section

public:
    // Constructor: Initializes UserManager and loads data from the specified file.
    explicit UserManager(const std::string& filename = "users.json");
    // Saves current user data to the JSON file.
    void saveToFile() const; // Made public for external use

    // Checks if a user with the given username exists.
    bool userExists(const std::string& username) const;
    // Registers a new user with the provided username and password.
    bool registerUser(const std::string& username, const std::string& password);
    // Authenticates a user with the given username and password.
    bool authenticateUser(const std::string& username, const std::string& password) const;

    // Retrieves a mutable User object by username.
    std::optional<std::reference_wrapper<User>> getUser(const std::string& username);
    // Retrieves a const User object by username.
    std::optional<std::reference_wrapper<const User>> getUser(const std::string& username) const;

    // Sends a friend request from one user to another.
    bool sendFriendRequest(const std::string& from, const std::string& to);
    // Accepts a friend request.
    bool acceptFriendRequest(const std::string& username, const std::string& from);
    // Rejects a friend request.
    bool rejectFriendRequest(const std::string& rejecting_username, const std::string& sender_username);

    // Gets pending incoming friend requests for a user.
    std::optional<std::reference_wrapper<const std::unordered_set<std::string>>> getIncomingFriendRequests(const std::string& username) const;

    // Stores a chat message between two users.
    void storeMessage(const std::string& sender, const std::string& receiver, const std::string& content);
};

#endif // USER_MANAGER_HPP
