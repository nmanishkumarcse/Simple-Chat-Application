#pragma once

#include "ChatServer.h"

#include <string>

class ClientHandler {
public:
    ClientHandler(ChatServer& server, ClientSession& session);
    bool handle_line(const std::string& line);

private:
    void help();
    bool cmd_register(const std::string& rest);
    bool cmd_login(const std::string& rest);
    void cmd_users();
    void cmd_msg(const std::string& rest);
    void cmd_broadcast(const std::string& rest);
    void cmd_create_room(const std::string& rest);
    void cmd_join(const std::string& rest);
    void cmd_leave();
    void cmd_history();
    void cmd_rooms();
    void cmd_logout();
    void cmd_stats();
    void cmd_kick(const std::string& rest);
    void chat_text(const std::string& text);
    bool require_login();
    std::string now_stamp();

    ChatServer& server_;
    ClientSession& session_;
};
