#include "Database.h"

#include <iostream>
#include <unistd.h>

static int failures = 0;

static void expect(bool cond, const char* name) {
    if (cond) {
        std::cout << "PASS " << name << "\n";
    } else {
        std::cout << "FAIL " << name << "\n";
        ++failures;
    }
}

int main() {
    std::string path = "tests/db_test.tmp.db";
    unlink(path.c_str());
    Database db(path);
    expect(db.ok(), "open");
    expect(db.create_user("alice", "hash", true), "create user");
    expect(db.find_user("alice").has_value(), "find user");
    expect(db.create_room("lobby"), "create room");
    expect(db.add_member("lobby", "alice"), "join room");
    expect(db.save_message("alice", "", "lobby", "hello"), "save message");
    auto hist = db.recent_history("alice", 10);
    expect(!hist.empty(), "history not empty");
    unlink(path.c_str());
    return failures == 0 ? 0 : 1;
}
