#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

struct Message {
    std::string sender;
    std::string content;
};

class User {
public:
friend class UserManager;

User(const std::string& username, const std::string& password);

    const std::string& getUsername() const;
    bool checkPassword(const std::string& password) const;

    // Friend request lifecycle
    bool hasFriend(const std::string& other) const;
    bool hasPendingRequestFrom(const std::string& other) const;
    bool hasSentRequestTo(const std::string& other) const;

    void sendFriendRequestTo(const std::string& other);
    void receiveFriendRequestFrom(const std::string& other);
    bool acceptFriendRequestFrom(const std::string& other);
    // Modify signature to take sender's username
    void rejectFriendRequestFrom(const std::string& sender_username);
    void completeOutgoingFriendRequest(const std::string& other); // New method
    void cancelOutgoingFriendRequest(const std::string& other); // New method to cancel an outgoing request

    // Friends and messaging
    const std::unordered_set<std::string>& getFriends() const;
    const std::unordered_set<std::string>& getIncomingFriendRequests() const;
    void storeMessage(const std::string& chatPartner, const std::string& sender, const std::string& content);
    const std::vector<Message>& getChatHistoryWith(const std::string& friendUsername) const;

private:
    std::string username;
    std::string passwordHash;

    std::unordered_set<std::string> friends;
    std::unordered_set<std::string> incomingRequests;
    std::unordered_set<std::string> outgoingRequests;



    std::unordered_map<std::string, std::vector<Message>> chatHistory;
};

#endif
