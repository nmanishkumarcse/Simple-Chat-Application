#pragma once

#include "Database.h"

#include <string>
#include <vector>

class RoomManager {
public:
    explicit RoomManager(Database& db);

    std::string create_room(const std::string& name);
    std::string join_room(const std::string& username, const std::string& name);
    std::string leave_room(const std::string& username, const std::string& name);
    std::vector<std::string> list_rooms();

private:
    Database& db_;
};
