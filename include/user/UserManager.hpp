#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include "User.hpp"
#include <unordered_map>
#include <string>
#include <optional>

class UserManager {
private:
    std::unordered_map<std::string, User> users;
    std::string dataFile;

    void loadFromFile();
    void saveToFile() const;

public:
    explicit UserManager(const std::string& filename = "users.json");

    bool userExists(const std::string& username) const;
    bool registerUser(const std::string& username, const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password) const;

    std::optional<std::reference_wrapper<User>> getUser(const std::string& username);
    std::optional<std::reference_wrapper<const User>> getUser(const std::string& username) const;

    bool sendFriendRequest(const std::string& from, const std::string& to);
    bool acceptFriendRequest(const std::string& username, const std::string& from);

    void storeMessage(const std::string& sender, const std::string& receiver, const std::string& content);
};

#endif // USER_MANAGER_HPP
