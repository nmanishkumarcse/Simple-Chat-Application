#include "MessageManager.h"

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
    std::string path = "tests/msg_test.tmp.db";
    unlink(path.c_str());
    Database db(path);
    MessageManager mm(db);
    expect(db.create_user("alice", "h", false), "user");
    expect(mm.store("alice", "", "", "hi"), "store broadcast");
    expect(!mm.history("alice", 5).empty(), "history");
    unlink(path.c_str());
    return failures == 0 ? 0 : 1;
}
