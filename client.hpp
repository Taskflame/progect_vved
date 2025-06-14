#pragma once
#include <string>
class ChatClient {
private:
    int sock = -1;
    std::string role = "user";
public:
    ~ChatClient();
    void connectToServer(const std::string& ip, int port, const std::string& password = "");
    void receiveMessages();
    void sendMessage(const std::string& message);
    std::string getRole() const { return role; }
};