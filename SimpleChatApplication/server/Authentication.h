#pragma once

#include "Database.h"

#include <string>

class Authentication {
public:
    explicit Authentication(Database& db);

    // Hash is SHA-256(username + ":" + password). We never store the raw password.
    static std::string hash_password(const std::string& username, const std::string& password);

    bool valid_username(const std::string& username) const;
    std::string register_user(const std::string& username, const std::string& password);
    std::string login_user(const std::string& username, const std::string& password);

private:
    Database& db_;
};
