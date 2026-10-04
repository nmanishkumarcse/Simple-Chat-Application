#include "ChatServer.h"
#include "common.hpp"

#include <cstdlib>
#include <iostream>
#include <sys/stat.h>

int main(int argc, char** argv) {
    int port = chat::kDefaultPort;
    if (argc >= 2) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Usage: " << argv[0] << " [port]\n";
            return 1;
        }
    }

    mkdir("database", 0755);

    ChatServer server(port, chat::kDbPath);
    if (!server.start()) {
        return 1;
    }
    server.run();
    return 0;
}
