#include "../include/user/User.hpp"
#include <functional>

User::User(const std::string& username, const std::string& password)
    : username(username), passwordHash(std::to_string(std::hash<std::string>{}(password))) {}

const std::string& User::getUsername() const {
    return username;
}

bool User::checkPassword(const std::string& password) const {
    return passwordHash == std::to_string(std::hash<std::string>{}(password));
}

bool User::hasFriend(const std::string& other) const {
    return friends.count(other) > 0;
}

bool User::hasPendingRequestFrom(const std::string& other) const {
    return incomingRequests.count(other) > 0;
}

bool User::hasSentRequestTo(const std::string& other) const {
    return outgoingRequests.count(other) > 0;
}

void User::sendFriendRequestTo(const std::string& other) {
    outgoingRequests.insert(other);
}

void User::receiveFriendRequestFrom(const std::string& other) {
    incomingRequests.insert(other);
}

bool User::acceptFriendRequestFrom(const std::string& other) {
    if (incomingRequests.count(other)) {
        incomingRequests.erase(other);
        friends.insert(other);
        return true;
    }
    return false;
}

void User::rejectFriendRequestFrom(const std::string& other) {
    incomingRequests.erase(other);
}

void User::completeOutgoingFriendRequest(const std::string& other) {
    outgoingRequests.erase(other);
    friends.insert(other);
}

const std::unordered_set<std::string>& User::getFriends() const {
    return friends;
}

void User::storeMessage(const std::string& chatPartner, const std::string& sender, const std::string& content) {
    chatHistory[chatPartner].push_back({sender, content});
}

const std::vector<Message>& User::getChatHistoryWith(const std::string& friendUsername) const {
    static const std::vector<Message> empty;
    auto it = chatHistory.find(friendUsername);
    return it != chatHistory.end() ? it->second : empty;
}
