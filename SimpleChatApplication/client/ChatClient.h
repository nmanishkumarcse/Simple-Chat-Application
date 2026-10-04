#pragma once

#include <string>

class ChatClient {
public:
    ChatClient(std::string host, int port);
    ~ChatClient();

    bool connect_to_server();
    void run();

private:
    void recv_loop();

    std::string host_;
    int port_ = 5000;
    int fd_ = -1;
    bool running_ = true;
};
