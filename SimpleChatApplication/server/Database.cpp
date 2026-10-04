#include "Database.h"

#include <cstdlib>
#include <iostream>

namespace {

int query_int_callback(void* ptr, int /*argc*/, char** argv, char** /*names*/) {
    int* out = static_cast<int*>(ptr);
    *out = argv[0] ? std::atoi(argv[0]) : 0;
    return 0;
}

}  // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }

    // FOREIGN KEYS must be turned on in SQLite.
    exec("PRAGMA foreign_keys = ON;");

    const char* schema = R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            status TEXT NOT NULL DEFAULT 'offline',
            is_admin INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS rooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now'))
        );
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER NOT NULL,
            receiver_id INTEGER,
            room_id INTEGER,
            message TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            FOREIGN KEY (sender_id) REFERENCES users(id),
            FOREIGN KEY (receiver_id) REFERENCES users(id),
            FOREIGN KEY (room_id) REFERENCES rooms(id)
        );
        CREATE TABLE IF NOT EXISTS room_members (
            room_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY (room_id, user_id),
            FOREIGN KEY (room_id) REFERENCES rooms(id),
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
    )SQL";

    if (!exec(schema)) {
        std::cerr << "Failed to create tables: " << last_error_ << "\n";
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        last_error_ = err ? err : "sqlite error";
        sqlite3_free(err);
        return false;
    }
    return true;
}

std::optional<int> Database::user_id(const std::string& username) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM users WHERE username = ?1;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<int> id;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

bool Database::create_user(const std::string& username, const std::string& password_hash, bool make_admin) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO users(username, password_hash, is_admin) VALUES(?1, ?2, ?3);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, make_admin ? 1 : 0);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

std::optional<UserRow> Database::find_user(const std::string& username) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, username, password_hash, is_admin, status FROM users WHERE username = ?1;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<UserRow> row;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRow u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.is_admin = sqlite3_column_int(stmt, 3);
        u.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        row = u;
    }
    sqlite3_finalize(stmt);
    return row;
}

bool Database::set_status(const std::string& username, const std::string& status) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE users SET status = ?1 WHERE username = ?2;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::set_admin(const std::string& username, bool is_admin) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE users SET is_admin = ?1 WHERE username = ?2;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int(stmt, 1, is_admin ? 1 : 0);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int Database::user_count() {
    int count = 0;
    char* err = nullptr;
    sqlite3_exec(db_, "SELECT COUNT(*) FROM users;", query_int_callback, &count, &err);
    sqlite3_free(err);
    return count;
}

bool Database::create_room(const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO rooms(name) VALUES(?1);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

std::optional<int> Database::room_id(const std::string& name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM rooms WHERE name = ?1;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<int> id;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

bool Database::add_member(const std::string& room, const std::string& username) {
    auto rid = room_id(room);
    auto uid = user_id(username);
    if (!rid || !uid) {
        last_error_ = "unknown room or user";
        return false;
    }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR IGNORE INTO room_members(room_id, user_id) VALUES(?1, ?2);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int(stmt, 1, *rid);
    sqlite3_bind_int(stmt, 2, *uid);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::remove_member(const std::string& room, const std::string& username) {
    auto rid = room_id(room);
    auto uid = user_id(username);
    if (!rid || !uid) {
        return false;
    }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM room_members WHERE room_id = ?1 AND user_id = ?2;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, *rid);
    sqlite3_bind_int(stmt, 2, *uid);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return true;
}

std::vector<std::string> Database::list_rooms() {
    std::vector<std::string> rooms;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT name FROM rooms ORDER BY name;", -1, &stmt, nullptr) != SQLITE_OK) {
        return rooms;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rooms.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return rooms;
}

bool Database::save_message(const std::string& sender,
                            const std::string& receiver,
                            const std::string& room,
                            const std::string& text) {
    auto sid = user_id(sender);
    if (!sid) {
        return false;
    }
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO messages(sender_id, receiver_id, room_id, message) VALUES(?1, ?2, ?3, ?4);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int(stmt, 1, *sid);
    if (receiver.empty()) {
        sqlite3_bind_null(stmt, 2);
    } else {
        auto rid = user_id(receiver);
        if (!rid) {
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_bind_int(stmt, 2, *rid);
    }
    if (room.empty()) {
        sqlite3_bind_null(stmt, 3);
    } else {
        auto roomid = room_id(room);
        if (!roomid) {
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_bind_int(stmt, 3, *roomid);
    }
    sqlite3_bind_text(stmt, 4, text.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<MessageRow> Database::recent_history(const std::string& username, int limit) {
    std::vector<MessageRow> rows;
    auto uid = user_id(username);
    if (!uid) {
        return rows;
    }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"SQL(
        SELECT m.id, su.username,
               COALESCE(ru.username, ''),
               COALESCE(r.name, ''),
               m.message, m.created_at
        FROM messages m
        JOIN users su ON su.id = m.sender_id
        LEFT JOIN users ru ON ru.id = m.receiver_id
        LEFT JOIN rooms r ON r.id = m.room_id
        WHERE m.sender_id = ?1
           OR m.receiver_id = ?1
           OR (m.receiver_id IS NULL AND m.room_id IS NULL)
           OR m.room_id IN (SELECT room_id FROM room_members WHERE user_id = ?1)
        ORDER BY m.id DESC
        LIMIT ?2;
    )SQL";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return rows;
    }
    sqlite3_bind_int(stmt, 1, *uid);
    sqlite3_bind_int(stmt, 2, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MessageRow m;
        m.id = sqlite3_column_int(stmt, 0);
        m.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        m.receiver = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        m.room = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        m.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        m.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        rows.push_back(m);
    }
    sqlite3_finalize(stmt);
    return rows;
}
