#include "ChatClient.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>

ChatClient::ChatClient(std::string host, int port) : host_(std::move(host)), port_(port) {}

ChatClient::~ChatClient() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool ChatClient::connect_to_server() {
    // socket() = pick up the telephone.
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Bad IP address: " << host_ << " (use 127.0.0.1 for this computer)\n";
        return false;
    }

    // connect() = dial the server's IP + port.
    if (connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect");
        return false;
    }
    return true;
}

void ChatClient::recv_loop() {
    char buf[512];
    while (running_) {
        ssize_t n = recv(fd_, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            std::cout << "\nDisconnected from server.\n";
            running_ = false;
            break;
        }
        buf[n] = '\0';
        std::cout << buf;
        std::cout.flush();
    }
}

void ChatClient::run() {
    std::thread reader(&ChatClient::recv_loop, this);
    std::string line;
    while (running_ && std::getline(std::cin, line)) {
        line += '\n';
        if (send(fd_, line.c_str(), line.size(), MSG_NOSIGNAL) < 0) {
            break;
        }
        if (line == "/quit\n") {
            running_ = false;
            break;
        }
    }
    running_ = false;
    shutdown(fd_, SHUT_RDWR);
    if (reader.joinable()) {
        reader.join();
    }
}
