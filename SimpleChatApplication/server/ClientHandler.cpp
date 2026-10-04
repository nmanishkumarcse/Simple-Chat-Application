#include "ClientHandler.h"

#include "common.hpp"

#include <ctime>
#include <sstream>

ClientHandler::ClientHandler(ChatServer& server, ClientSession& session)
    : server_(server), session_(session) {}

std::string ClientHandler::now_stamp() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

void ClientHandler::help() {
    server_.send_line(session_.fd,
                      "Commands: /help /register /login /users /msg /broadcast "
                      "/create_room /join /leave /rooms /history /logout /quit "
                      "/stats /kick");
}

bool ClientHandler::require_login() {
    if (!session_.logged_in) {
        server_.send_line(session_.fd, "ERROR please /login first");
        return false;
    }
    return true;
}

bool ClientHandler::cmd_register(const std::string& rest) {
    std::istringstream in(rest);
    std::string user, pass;
    in >> user >> pass;
    if (user.empty() || pass.empty()) {
        server_.send_line(session_.fd, "Usage: /register username password");
        return true;
    }
    server_.send_line(session_.fd, server_.auth().register_user(user, pass));
    return true;
}

bool ClientHandler::cmd_login(const std::string& rest) {
    std::istringstream in(rest);
    std::string user, pass;
    in >> user >> pass;
    if (user.empty() || pass.empty()) {
        server_.send_line(session_.fd, "Usage: /login username password");
        return true;
    }
    if (session_.logged_in) {
        server_.send_line(session_.fd, "ERROR already logged in");
        return true;
    }
    for (const auto& name : server_.online_users()) {
        if (name == user) {
            server_.send_line(session_.fd, "ERROR user already online");
            return true;
        }
    }
    std::string result = server_.auth().login_user(user, pass);
    server_.send_line(session_.fd, result);
    if (result.rfind("OK", 0) == 0) {
        auto row = server_.db().find_user(user);
        session_.logged_in = true;
        session_.username = user;
        session_.is_admin = row && row->is_admin;
        server_.broadcast("[server] " + user + " is online", session_.fd);
        server_.write_monitor();
    }
    return true;
}

void ClientHandler::cmd_users() {
    if (!require_login()) {
        return;
    }
    auto names = server_.online_users();
    std::ostringstream out;
    out << "Online:";
    for (const auto& n : names) {
        out << " " << n;
    }
    server_.send_line(session_.fd, out.str());
}

void ClientHandler::cmd_msg(const std::string& rest) {
    if (!require_login()) {
        return;
    }
    auto space = rest.find(' ');
    if (space == std::string::npos) {
        server_.send_line(session_.fd, "Usage: /msg username message");
        return;
    }
    std::string target = rest.substr(0, space);
    std::string text = rest.substr(space + 1);
    if (target.empty() || text.empty()) {
        server_.send_line(session_.fd, "Usage: /msg username message");
        return;
    }
    if (!server_.db().find_user(target)) {
        server_.send_line(session_.fd, "ERROR no such user");
        return;
    }
    std::string line = "[" + now_stamp() + "] (private) " + session_.username + " -> " + target + ": " + text;
    server_.send_line(session_.fd, line);
    server_.send_to_username(target, line);
    server_.messages().store(session_.username, target, "", text);
    server_.bump_messages();
}

void ClientHandler::cmd_broadcast(const std::string& rest) {
    if (!require_login()) {
        return;
    }
    if (rest.empty()) {
        server_.send_line(session_.fd, "Usage: /broadcast message");
        return;
    }
    std::string line = "[" + now_stamp() + "] " + session_.username + ": " + rest;
    server_.send_line(session_.fd, line);
    server_.broadcast(line, session_.fd);
    server_.messages().store(session_.username, "", "", rest);
    server_.bump_messages();
}

void ClientHandler::cmd_create_room(const std::string& rest) {
    if (!require_login()) {
        return;
    }
    std::istringstream in(rest);
    std::string name;
    in >> name;
    server_.send_line(session_.fd, server_.rooms().create_room(name));
}

void ClientHandler::cmd_join(const std::string& rest) {
    if (!require_login()) {
        return;
    }
    std::istringstream in(rest);
    std::string name;
    in >> name;
    std::string result = server_.rooms().join_room(session_.username, name);
    server_.send_line(session_.fd, result);
    if (result.rfind("OK", 0) == 0) {
        session_.current_room = name;
        server_.broadcast("[server] " + session_.username + " joined #" + name, session_.fd);
    }
}

void ClientHandler::cmd_leave() {
    if (!require_login()) {
        return;
    }
    if (session_.current_room.empty()) {
        server_.send_line(session_.fd, "ERROR you are not in a room");
        return;
    }
    std::string name = session_.current_room;
    server_.send_line(session_.fd, server_.rooms().leave_room(session_.username, name));
    session_.current_room.clear();
}

void ClientHandler::cmd_rooms() {
    if (!require_login()) {
        return;
    }
    auto rooms = server_.rooms().list_rooms();
    std::ostringstream out;
    out << "Rooms:";
    for (const auto& r : rooms) {
        out << " " << r;
    }
    if (rooms.empty()) {
        out << " (none)";
    }
    server_.send_line(session_.fd, out.str());
}

void ClientHandler::cmd_history() {
    if (!require_login()) {
        return;
    }
    auto rows = server_.messages().history(session_.username, chat::kHistoryLimit);
    if (rows.empty()) {
        server_.send_line(session_.fd, "No history yet.");
        return;
    }
    for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
        std::ostringstream out;
        out << "[" << it->created_at << "] " << it->sender;
        if (!it->receiver.empty()) {
            out << " -> " << it->receiver;
        }
        if (!it->room.empty()) {
            out << " #" << it->room;
        }
        out << ": " << it->text;
        server_.send_line(session_.fd, out.str());
    }
}

void ClientHandler::cmd_logout() {
    if (!require_login()) {
        return;
    }
    server_.db().set_status(session_.username, "offline");
    server_.broadcast("[server] " + session_.username + " logged out", session_.fd);
    session_.logged_in = false;
    session_.username.clear();
    session_.current_room.clear();
    session_.is_admin = false;
    server_.send_line(session_.fd, "OK logged out");
    server_.write_monitor();
}

void ClientHandler::cmd_stats() {
    if (!require_login()) {
        return;
    }
    auto names = server_.online_users();
    std::ostringstream out;
    out << "stats online=" << names.size();
    if (session_.is_admin) {
        out << " (you are admin)";
    }
    server_.send_line(session_.fd, out.str());
}

void ClientHandler::cmd_kick(const std::string& rest) {
    if (!require_login()) {
        return;
    }
    if (!session_.is_admin) {
        server_.send_line(session_.fd, "ERROR admin only");
        return;
    }
    std::istringstream in(rest);
    std::string user;
    in >> user;
    if (user.empty()) {
        server_.send_line(session_.fd, "Usage: /kick username");
        return;
    }
    server_.disconnect_username(user);
    server_.send_line(session_.fd, "OK kick requested for " + user);
}

void ClientHandler::chat_text(const std::string& text) {
    if (!require_login()) {
        return;
    }
    if (!session_.current_room.empty()) {
        std::string line = "[" + now_stamp() + "] #" + session_.current_room + " " + session_.username + ": " + text;
        server_.send_line(session_.fd, line);
        server_.send_to_room(session_.current_room, line, session_.fd);
        server_.messages().store(session_.username, "", session_.current_room, text);
        server_.bump_messages();
        return;
    }
    cmd_broadcast(text);
}

bool ClientHandler::handle_line(const std::string& line) {
    if (line == "/quit") {
        server_.send_line(session_.fd, "Goodbye.");
        return false;
    }
    if (line == "/help") {
        help();
        return true;
    }
    if (line.rfind("/register ", 0) == 0) {
        return cmd_register(line.substr(10));
    }
    if (line.rfind("/login ", 0) == 0) {
        return cmd_login(line.substr(7));
    }
    if (line == "/users") {
        cmd_users();
        return true;
    }
    if (line.rfind("/msg ", 0) == 0) {
        cmd_msg(line.substr(5));
        return true;
    }
    if (line.rfind("/broadcast ", 0) == 0) {
        cmd_broadcast(line.substr(11));
        return true;
    }
    if (line.rfind("/create_room ", 0) == 0) {
        cmd_create_room(line.substr(13));
        return true;
    }
    if (line.rfind("/join ", 0) == 0) {
        cmd_join(line.substr(6));
        return true;
    }
    if (line == "/leave") {
        cmd_leave();
        return true;
    }
    if (line == "/rooms") {
        cmd_rooms();
        return true;
    }
    if (line == "/history") {
        cmd_history();
        return true;
    }
    if (line == "/logout") {
        cmd_logout();
        return true;
    }
    if (line == "/stats") {
        cmd_stats();
        return true;
    }
    if (line.rfind("/kick ", 0) == 0) {
        cmd_kick(line.substr(6));
        return true;
    }
    if (!line.empty() && line[0] == '/') {
        server_.send_line(session_.fd, "ERROR unknown command. Try /help");
        return true;
    }
    chat_text(line);
    return true;
}
