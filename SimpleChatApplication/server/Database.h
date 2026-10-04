#pragma once

// Database.h
// SQLite is a small database stored in ONE file (database/chat.db).
// Think of a table as a spreadsheet:
//   - each row is one record
//   - a PRIMARY KEY uniquely identifies a row (usually "id")
//   - a FOREIGN KEY points at a row in another table

#include <sqlite3.h>

#include <optional>
#include <string>
#include <vector>

struct UserRow {
    int id = 0;
    std::string username;
    std::string password_hash;
    int is_admin = 0;
    std::string status;
};

struct MessageRow {
    int id = 0;
    std::string sender;
    std::string receiver;  // empty if not a private message
    std::string room;      // empty if not a room message
    std::string text;
    std::string created_at;
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool ok() const { return db_ != nullptr; }
    std::string last_error() const { return last_error_; }

    bool create_user(const std::string& username, const std::string& password_hash, bool make_admin);
    std::optional<UserRow> find_user(const std::string& username);
    bool set_status(const std::string& username, const std::string& status);
    bool set_admin(const std::string& username, bool is_admin);
    int user_count();

    bool create_room(const std::string& name);
    std::optional<int> room_id(const std::string& name);
    bool add_member(const std::string& room, const std::string& username);
    bool remove_member(const std::string& room, const std::string& username);
    std::vector<std::string> list_rooms();

    bool save_message(const std::string& sender,
                      const std::string& receiver,
                      const std::string& room,
                      const std::string& text);
    std::vector<MessageRow> recent_history(const std::string& username, int limit);

private:
    bool exec(const std::string& sql);
    std::optional<int> user_id(const std::string& username);
    sqlite3* db_ = nullptr;
    std::string last_error_;
};
