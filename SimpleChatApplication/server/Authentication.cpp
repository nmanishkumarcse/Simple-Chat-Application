#include "Authentication.h"

#include "sha256.hpp"

#include <cctype>

Authentication::Authentication(Database& db) : db_(db) {}

std::string Authentication::hash_password(const std::string& username, const std::string& password) {
    return chat::Sha256::hex(username + ":" + password);
}

bool Authentication::valid_username(const std::string& username) const {
    if (username.size() < 3 || username.size() > 20) {
        return false;
    }
    for (char c : username) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) {
            return false;
        }
    }
    return true;
}

std::string Authentication::register_user(const std::string& username, const std::string& password) {
    if (!valid_username(username)) {
        return "ERROR usernames must be 3-20 letters, digits, or underscore";
    }
    if (password.size() < 3) {
        return "ERROR password must be at least 3 characters";
    }
    if (db_.find_user(username)) {
        return "ERROR username already exists";
    }
    bool first_user = db_.user_count() == 0;
    if (!db_.create_user(username, hash_password(username, password), first_user)) {
        return "ERROR could not create user";
    }
    if (first_user) {
        return "OK registered as admin (first user)";
    }
    return "OK registered";
}

std::string Authentication::login_user(const std::string& username, const std::string& password) {
    auto user = db_.find_user(username);
    if (!user) {
        return "ERROR unknown user";
    }
    if (user->password_hash != hash_password(username, password)) {
        return "ERROR bad password";
    }
    db_.set_status(username, "online");
    return "OK login";
}
