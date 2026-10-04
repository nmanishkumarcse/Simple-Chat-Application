#include "RoomManager.h"

#include <cctype>

RoomManager::RoomManager(Database& db) : db_(db) {}

std::string RoomManager::create_room(const std::string& name) {
    if (name.size() < 2 || name.size() > 20) {
        return "ERROR room name must be 2-20 characters";
    }
    for (char c : name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) {
            return "ERROR room name must be letters, digits, or underscore";
        }
    }
    if (db_.room_id(name)) {
        return "ERROR room already exists";
    }
    if (!db_.create_room(name)) {
        return "ERROR could not create room";
    }
    return "OK room created: " + name;
}

std::string RoomManager::join_room(const std::string& username, const std::string& name) {
    if (!db_.room_id(name)) {
        return "ERROR no such room";
    }
    if (!db_.add_member(name, username)) {
        return "ERROR could not join";
    }
    return "OK joined " + name;
}

std::string RoomManager::leave_room(const std::string& username, const std::string& name) {
    db_.remove_member(name, username);
    return "OK left " + name;
}

std::vector<std::string> RoomManager::list_rooms() {
    return db_.list_rooms();
}
