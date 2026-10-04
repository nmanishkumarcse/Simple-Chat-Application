#pragma once

#include "Database.h"

#include <string>
#include <vector>

class MessageManager {
public:
    explicit MessageManager(Database& db);

    bool store(const std::string& sender,
               const std::string& receiver,
               const std::string& room,
               const std::string& text);
    std::vector<MessageRow> history(const std::string& username, int limit);

private:
    Database& db_;
};
