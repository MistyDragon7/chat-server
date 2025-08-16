#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// Represents a single chat message.
struct Message {
    std::string sender;  // Username of the message sender.
    std::string content; // Content of the message.
};

// Represents a chat user with their profile, friends, and chat history.
class User {
public:
    // Grants UserManager access to private members for data management.
    friend class UserManager;

    // Constructor: Initializes a User with a username and password.
    User(const std::string& username, const std::string& password);

    // Returns the username of the user.
    const std::string& getUsername() const;
    // Checks if the provided password matches the user's stored password hash.
    bool checkPassword(const std::string& password) const;

    // Checks if the user is friends with another user.
    bool hasFriend(const std::string& other) const;
    // Checks if there is a pending friend request from another user.
    bool hasPendingRequestFrom(const std::string& other) const;
    // Checks if the user has sent a friend request to another user.
    bool hasSentRequestTo(const std::string& other) const;

    // Adds another user to the outgoing friend requests.
    void sendFriendRequestTo(const std::string& other);
    // Adds another user to the incoming friend requests.
    void receiveFriendRequestFrom(const std::string& other);
    // Accepts a pending friend request from another user.
    bool acceptFriendRequestFrom(const std::string& other);
    // Rejects a pending friend request from a specified sender.
    void rejectFriendRequestFrom(const std::string& sender_username);
    // Marks an outgoing friend request as completed (accepted).
    void completeOutgoingFriendRequest(const std::string& other);
    // Cancels an outgoing friend request.
    void cancelOutgoingFriendRequest(const std::string& other);

    // Returns a constant reference to the set of friends.
    const std::unordered_set<std::string>& getFriends() const;
    // Returns a constant reference to the set of incoming friend requests.
    const std::unordered_set<std::string>& getIncomingFriendRequests() const;
    // Stores a message in the chat history with a specific partner.
    void storeMessage(const std::string& chatPartner, const std::string& sender, const std::string& content);
    // Returns a constant reference to the chat history with a specific friend.
    const std::vector<Message>& getChatHistoryWith(const std::string& friendUsername) const;

private:
    std::string username;     // User's unique username.
    std::string passwordHash; // Hashed password for authentication.

    std::unordered_set<std::string> friends;         // Set of friends' usernames.
    std::unordered_set<std::string> incomingRequests; // Set of incoming friend requests.
    std::unordered_set<std::string> outgoingRequests; // Set of outgoing friend requests.

    // Stores chat history with different friends.
    std::unordered_map<std::string, std::vector<Message>> chatHistory;
};

#endif
