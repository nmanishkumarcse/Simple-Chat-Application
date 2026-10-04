#include "ChatClient.h"
#include "common.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = chat::kDefaultPort;
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = std::atoi(argv[2]);
    }
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ip> [port]\n"
                  << "Example: " << argv[0] << " 127.0.0.1 5000\n";
        return 1;
    }

    ChatClient client(host, port);
    if (!client.connect_to_server()) {
        return 1;
    }
    std::cout << "Connected. Type /help\n";
    client.run();
    return 0;
}
