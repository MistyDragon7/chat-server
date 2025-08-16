#include "../include/user/User.hpp"
#include <functional>

// Constructor: Initializes a User object with a username and a hashed password.
User::User(const std::string& username, const std::string& password)
    : username(username), passwordHash(std::to_string(std::hash<std::string>{}(password))) {}

// Returns the username of the user.
const std::string& User::getUsername() const {
    return username;
}

// Checks if the provided password matches the stored hashed password.
bool User::checkPassword(const std::string& password) const {
    return passwordHash == std::to_string(std::hash<std::string>{}(password));
}

// Checks if the user is friends with the specified other user.
bool User::hasFriend(const std::string& other) const {
    return friends.count(other) > 0;
}

// Checks if there is a pending friend request from the specified other user.
bool User::hasPendingRequestFrom(const std::string& other) const {
    return incomingRequests.count(other) > 0;
}

// Checks if the user has sent a friend request to the specified other user.
bool User::hasSentRequestTo(const std::string& other) const {
    return outgoingRequests.count(other) > 0;
}

// Adds the specified user to the outgoing friend requests.
void User::sendFriendRequestTo(const std::string& other) {
    outgoingRequests.insert(other);
}

// Adds the specified user to the incoming friend requests.
void User::receiveFriendRequestFrom(const std::string& other) {
    incomingRequests.insert(other);
}

// Accepts a pending friend request from the specified other user.
bool User::acceptFriendRequestFrom(const std::string& other) {
    if (incomingRequests.count(other)) {
        incomingRequests.erase(other);
        friends.insert(other);
        return true;
    }
    return false;
}

// Rejects a pending friend request from the specified sender.
void User::rejectFriendRequestFrom(const std::string& sender_username) {
    incomingRequests.erase(sender_username);
    // Also remove from the sender's outgoing requests if this method is called directly by UserManager
    // This part will be handled in UserManager, which has access to both users
}

// Marks an outgoing friend request as completed and adds the user to friends.
void User::completeOutgoingFriendRequest(const std::string& other) {
    outgoingRequests.erase(other);
    friends.insert(other);
}

// Cancels an outgoing friend request.
void User::cancelOutgoingFriendRequest(const std::string& other) {
    outgoingRequests.erase(other);
}

// Returns a constant reference to the set of friends.
const std::unordered_set<std::string>& User::getFriends() const {
    return friends;
}

// Returns a constant reference to the set of incoming friend requests.
const std::unordered_set<std::string>& User::getIncomingFriendRequests() const {
    return incomingRequests;
}

// Stores a message in the chat history with a specific partner.
void User::storeMessage(const std::string& chatPartner, const std::string& sender, const std::string& content) {
    chatHistory[chatPartner].push_back({sender, content});
}

// Returns a constant reference to the chat history with a specific friend.
const std::vector<Message>& User::getChatHistoryWith(const std::string& friendUsername) const {
    static const std::vector<Message> empty;
    auto it = chatHistory.find(friendUsername);
    return it != chatHistory.end() ? it->second : empty;
}
