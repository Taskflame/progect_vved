#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <unordered_map>
#include <set>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class ChatParticipant;

class ChatServer {
private:
    int server_fd;
    sockaddr_in server_addr;
    std::vector<ChatParticipant*> participants;
    std::mutex participants_mutex;
    std::set<std::string> banned_ips;
    std::ofstream log_file;
    std::string admin_password = "admin123";

public:
    explicit ChatServer(int port);
    ~ChatServer();
    void broadcast(const std::string& message, int sender_socket = -1);
    void run();
    void addParticipant(ChatParticipant* participant);
    void removeParticipant(int socket);
    bool isBanned(const std::string& ip);
    void kick(const std::string& ip);
    void ban(const std::string& ip);
    std::string listParticipants();
    void logMessage(const std::string& ip, const std::string& message);
};

class ChatParticipant {
protected:
    ChatServer* server;
    int socket;
    std::string role;
    std::string address;

public:
    ChatParticipant(ChatServer* srv, int sock, const std::string& addr, const std::string& r);
    virtual ~ChatParticipant();
    virtual void handle() = 0;
    std::string getRole() const { return role; }
    int getSocket() const { return socket; }
    std::string getAddress() const { return address; }
};

class UserParticipant : public ChatParticipant {
public:
    UserParticipant(ChatServer* srv, int sock, const std::string& addr, const std::string& r);
    void handle() override;
};