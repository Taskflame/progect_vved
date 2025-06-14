#include "server.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <ctime>

ChatParticipant::ChatParticipant(ChatServer* srv, int sock, const std::string& addr, const std::string& r)
    : server(srv), socket(sock), address(addr), role(r) {}

ChatParticipant::~ChatParticipant() {
    close(socket);
}

UserParticipant::UserParticipant(ChatServer* srv, int sock, const std::string& addr, const std::string& r)
    : ChatParticipant(srv, sock, addr, r) {}

void UserParticipant::handle() {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        std::string msg(buffer, bytes);
        if (msg.back() == '\n') msg.pop_back();

        if (msg == "exit") break;

        if (server->isBanned(address)) {
            std::string ban_msg = "[SERVER] Ваш IP заблокирован.\n";
            send(socket, ban_msg.c_str(), ban_msg.size(), 0);
            break;
        }

        if (role == "admin") {
            if (msg.find("/kick") == 0) {
                std::string ip = msg.substr(6);
                server->kick(ip);
                continue;
            } else if (msg.find("/ban") == 0) {
                std::string ip = msg.substr(5);
                server->ban(ip);
                continue;
            } else if (msg == "/list") {
                std::string list = server->listParticipants();
                send(socket, list.c_str(), list.size(), 0);
                continue;
            }
        }

        server->logMessage(address, msg);
        std::string formatted = "[" + address + "] " + msg + "\n";
        server->broadcast(formatted, socket);
    }

    server->removeParticipant(socket);
}

ChatServer::ChatServer(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    log_file.open("chat_log.txt", std::ios::app);
}

ChatServer::~ChatServer() {
    close(server_fd);
    log_file.close();
}

void ChatServer::broadcast(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> lock(participants_mutex);
    for (auto* p : participants) {
        if (p->getSocket() != sender_socket) {
            send(p->getSocket(), message.c_str(), message.size(), 0);
        }
    }
}

void ChatServer::run() {
    std::cout << "Сервер запущен на порту " << ntohs(server_addr.sin_port) << "\n";
    while (true) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_sock = accept(server_fd, (sockaddr*)&client_addr, &len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        std::string ip(ip_str);

        // 🔴 Лог подключения
        std::cout << "[LOG] Подключение клиента: " << ip << std::endl;
        log_file << "[LOG] Подключение клиента: " << ip << std::endl;

        char passbuf[128] = {0};
        int pass_len = recv(client_sock, passbuf, sizeof(passbuf), 0);
        std::string password = (pass_len > 0) ? std::string(passbuf, pass_len) : "";

        std::string role = (password == admin_password) ? "admin" : "user";
        std::string welcome = "[SERVER] Вы вошли как " + role + ".\n";
        send(client_sock, welcome.c_str(), welcome.size(), 0);

        ChatParticipant* participant = new UserParticipant(this, client_sock, ip, role);
        addParticipant(participant);
        std::thread([participant]() {
            participant->handle();
        }).detach();
    }
}

void ChatServer::addParticipant(ChatParticipant* p) {
    std::lock_guard<std::mutex> lock(participants_mutex);
    participants.push_back(p);
}

void ChatServer::removeParticipant(int socket) {
    std::lock_guard<std::mutex> lock(participants_mutex);
    participants.erase(std::remove_if(participants.begin(), participants.end(),
        [this, socket](ChatParticipant* p) {
            if (p->getSocket() == socket) {
                // 🔴 Лог отключения
                log_file << "[LOG] Отключение клиента: сокет " << socket << std::endl;
                delete p;
                return true;
            }
            return false;
        }), participants.end());
}

bool ChatServer::isBanned(const std::string& ip) {
    return banned_ips.find(ip) != banned_ips.end();
}

void ChatServer::kick(const std::string& ip) {
    std::lock_guard<std::mutex> lock(participants_mutex);
    log_file << "[LOG] Kick: " << ip << std::endl;

    for (auto* p : participants) {
        if (p->getAddress() == ip) {
            std::string msg = "[SERVER] Вы были отключены.\n";
            send(p->getSocket(), msg.c_str(), msg.size(), 0);
            close(p->getSocket());
        }
    }
}

void ChatServer::ban(const std::string& ip) {
    banned_ips.insert(ip);
    log_file << "[LOG] Ban: " << ip << std::endl;
    kick(ip);
}

std::string ChatServer::listParticipants() {
    std::lock_guard<std::mutex> lock(participants_mutex);
    std::string result = "[SERVER] Подключенные участники:\n";
    for (auto* p : participants) {
        result += p->getAddress() + " (" + p->getRole() + ")\n";
    }
    return result;
}

void ChatServer::logMessage(const std::string& ip, const std::string& message) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* tm_ptr = std::localtime(&now);
    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_ptr);

    log_file << "[" << time_buf << "] [" << ip << "] " << message << "\n";
    log_file.flush();
}
