#include "client.hpp"
#include <unistd.h>
#include <iostream>
#include <thread>

int main() {
    try {
        ChatClient client;
        std::cout << "Подключиться как (1) пользователь или (2) администратор? ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "2") {
            std::string password;
            std::cout << "Введите пароль администратора: ";
            std::getline(std::cin, password);
            client.connectToServer("127.0.0.1", 12345, password);
        } else {
            client.connectToServer("127.0.0.1", 12345);
        }

        std::thread recv_thread(&ChatClient::receiveMessages, &client);
        std::string msg;
        while (std::getline(std::cin, msg) && msg != "exit") {
            client.sendMessage(msg + "\n");
        }

        close(0); // Закрыть stdin
        recv_thread.join();
    } catch (...) {
        std::cerr << "Ошибка клиента\n";
    }
    return 0;
}