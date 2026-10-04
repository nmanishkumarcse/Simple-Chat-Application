#pragma once

#include "Authentication.h"
#include "Database.h"
#include "MessageManager.h"
#include "RoomManager.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// One connected TCP client.
struct ClientSession {
    int fd = -1;                 // file descriptor = the socket number Linux gave us
    std::string username;
    bool logged_in = false;
    bool is_admin = false;
    std::string current_room;
    std::string inbuf;           // leftover bytes until we see '\n'
};

class ChatServer {
public:
    ChatServer(int port, const std::string& db_path);
    ~ChatServer();

    bool start();
    void run();

    void send_line(int fd, const std::string& line);
    void broadcast(const std::string& line, int except_fd = -1);
    void send_to_room(const std::string& room, const std::string& line, int except_fd = -1);
    bool send_to_username(const std::string& username, const std::string& line);
    std::vector<std::string> online_users();
    void disconnect_username(const std::string& username);

    Database& db() { return db_; }
    Authentication& auth() { return auth_; }
    RoomManager& rooms() { return rooms_; }
    MessageManager& messages() { return messages_; }

    void bump_messages();
    void write_monitor();

    std::mutex& mutex() { return mu_; }

private:
    void accept_loop();
    void handle_client(ClientSession* session);
    void close_session(ClientSession* session);

    int port_ = 5000;
    int listen_fd_ = -1;
    Database db_;
    Authentication auth_;
    RoomManager rooms_;
    MessageManager messages_;
    std::mutex mu_;
    std::vector<ClientSession*> clients_;
    std::atomic<int> total_connections_{0};
    std::atomic<int> total_messages_{0};
    std::atomic<bool> running_{true};
};
