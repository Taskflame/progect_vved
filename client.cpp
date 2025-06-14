#include "client.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

ChatClient::~ChatClient() {
    if (sock != -1) close(sock);
}

void ChatClient::connectToServer(const std::string& ip, int port, const std::string& password) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    connect(sock, (sockaddr*)&addr, sizeof(addr));

    if (!password.empty()) {
        send(sock, password.c_str(), password.size(), 0);
        role = "admin";
    } else {
        role = "user";
    }
}

void ChatClient::receiveMessages() {
    char buf[1024];
    while (true) {
        int bytes = recv(sock, buf, sizeof(buf), 0);
        if (bytes <= 0) break;
        std::cout << std::string(buf, bytes);
    }
}

void ChatClient::sendMessage(const std::string& msg) {
    send(sock, msg.c_str(), msg.size(), 0);
}