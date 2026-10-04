#include "Authentication.h"
#include "Database.h"
#include "sha256.hpp"

#include <cstdio>
#include <iostream>
#include <string>
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
    expect(chat::Sha256::hex("abc") ==
               "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
           "sha256(abc)");

    std::string path = "tests/auth_test.tmp.db";
    unlink(path.c_str());
    Database db(path);
    expect(db.ok(), "open sqlite");
    Authentication auth(db);

    expect(auth.valid_username("alice") == true, "valid username");
    expect(auth.valid_username("ab") == false, "short username rejected");
    expect(auth.register_user("alice", "secret").rfind("OK", 0) == 0, "register alice as admin");
    expect(auth.register_user("alice", "secret").rfind("ERROR", 0) == 0, "duplicate rejected");
    expect(auth.login_user("alice", "secret").rfind("OK", 0) == 0, "login ok");
    expect(auth.login_user("alice", "wrong").rfind("ERROR", 0) == 0, "bad password");
    expect(auth.register_user("bob", "hunter2").rfind("OK", 0) == 0, "second user not admin");

    unlink(path.c_str());
    return failures == 0 ? 0 : 1;
}
