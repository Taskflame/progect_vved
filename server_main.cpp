#include "server.hpp"
int main() {
    ChatServer server(12345);
    server.run();
    return 0;
}