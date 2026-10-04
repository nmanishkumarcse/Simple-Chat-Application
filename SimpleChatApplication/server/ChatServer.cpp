#include "ChatServer.h"

#include "ClientHandler.h"
#include "common.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <thread>

ChatServer::ChatServer(int port, const std::string& db_path)
    : port_(port), db_(db_path), auth_(db_), rooms_(db_), messages_(db_) {}

ChatServer::~ChatServer() {
    running_ = false;
    if (listen_fd_ >= 0) {
        close(listen_fd_);
    }
    std::lock_guard<std::mutex> lock(mu_);
    for (ClientSession* s : clients_) {
        if (s->fd >= 0) {
            close(s->fd);
        }
    }
}

bool ChatServer::start() {
    if (!db_.ok()) {
        std::cerr << "Database failed: " << db_.last_error() << "\n";
        return false;
    }

    // socket() creates a communication endpoint. Think of it as buying a telephone
    // before anyone can call you.
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        perror("socket");
        return false;
    }

    int yes = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // listen on all local addresses
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    // bind() attaches the socket to a port (the "room number").
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    // listen() tells Linux: I am ready to receive calls.
    if (listen(listen_fd_, 16) < 0) {
        perror("listen");
        return false;
    }

    std::cout << "Chat server listening on port " << port_ << "\n";
    write_monitor();
    return true;
}

void ChatServer::run() {
    accept_loop();
}

void ChatServer::accept_loop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);

        // accept() waits for one client and returns a NEW socket for that client.
        int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_fd < 0) {
            if (!running_) {
                break;
            }
            perror("accept");
            continue;
        }

        total_connections_++;
        auto* session = new ClientSession();
        session->fd = client_fd;
        {
            std::lock_guard<std::mutex> lock(mu_);
            clients_.push_back(session);
        }
        write_monitor();

        // One thread per client. A thread is a separate line of work inside
        // the same program so several users can chat at the same time.
        std::thread(&ChatServer::handle_client, this, session).detach();
    }
}

void ChatServer::handle_client(ClientSession* session) {
    send_line(session->fd, "Welcome to SimpleChat. Type /help");
    ClientHandler handler(*this, *session);

    char buf[512];
    while (running_) {
        // recv() copies bytes from the TCP connection into our buffer.
        ssize_t n = recv(session->fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        session->inbuf.append(buf, static_cast<size_t>(n));
        std::string::size_type pos;
        while ((pos = session->inbuf.find('\n')) != std::string::npos) {
            std::string line = session->inbuf.substr(0, pos);
            session->inbuf.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }
            if (!handler.handle_line(line)) {
                close_session(session);
                return;
            }
        }
        if (session->inbuf.size() > static_cast<size_t>(chat::kMaxLine)) {
            send_line(session->fd, "ERROR line too long");
            session->inbuf.clear();
        }
    }
    close_session(session);
}

void ChatServer::close_session(ClientSession* session) {
    if (session->logged_in) {
        db_.set_status(session->username, "offline");
        broadcast("[server] " + session->username + " left", session->fd);
    }
    if (session->fd >= 0) {
        close(session->fd);
        session->fd = -1;
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if (*it == session) {
                clients_.erase(it);
                break;
            }
        }
    }
    write_monitor();
    delete session;
}

void ChatServer::send_line(int fd, const std::string& line) {
    if (fd < 0) {
        return;
    }
    std::string payload = line + "\n";
    // MSG_NOSIGNAL: do not crash the server if the other side already hung up.
    send(fd, payload.c_str(), payload.size(), MSG_NOSIGNAL);
}

void ChatServer::broadcast(const std::string& line, int except_fd) {
    std::lock_guard<std::mutex> lock(mu_);
    for (ClientSession* s : clients_) {
        if (s->fd >= 0 && s->logged_in && s->fd != except_fd) {
            send_line(s->fd, line);
        }
    }
}

void ChatServer::send_to_room(const std::string& room, const std::string& line, int except_fd) {
    std::lock_guard<std::mutex> lock(mu_);
    for (ClientSession* s : clients_) {
        if (s->fd >= 0 && s->logged_in && s->fd != except_fd && s->current_room == room) {
            send_line(s->fd, line);
        }
    }
}

bool ChatServer::send_to_username(const std::string& username, const std::string& line) {
    std::lock_guard<std::mutex> lock(mu_);
    for (ClientSession* s : clients_) {
        if (s->logged_in && s->username == username && s->fd >= 0) {
            send_line(s->fd, line);
            return true;
        }
    }
    return false;
}

std::vector<std::string> ChatServer::online_users() {
    std::vector<std::string> names;
    std::lock_guard<std::mutex> lock(mu_);
    for (ClientSession* s : clients_) {
        if (s->logged_in) {
            names.push_back(s->username);
        }
    }
    return names;
}

void ChatServer::disconnect_username(const std::string& username) {
    std::lock_guard<std::mutex> lock(mu_);
    for (ClientSession* s : clients_) {
        if (s->logged_in && s->username == username && s->fd >= 0) {
            send_line(s->fd, "You were kicked by an admin.");
            shutdown(s->fd, SHUT_RDWR);
        }
    }
}

void ChatServer::bump_messages() {
    total_messages_++;
    write_monitor();
}

void ChatServer::write_monitor() {
    // Best-effort: if the driver is not loaded, the server still works.
    int active = 0;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (ClientSession* s : clients_) {
            if (s->fd >= 0) {
                ++active;
            }
        }
    }
    std::ostringstream out;
    out << "status=running\n"
        << "active_clients=" << active << "\n"
        << "total_messages=" << total_messages_.load() << "\n"
        << "total_connections=" << total_connections_.load() << "\n";
    int fd = open(chat::kMonitorDevice, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return;
    }
    const std::string text = out.str();
    (void)write(fd, text.c_str(), text.size());
    close(fd);
}
