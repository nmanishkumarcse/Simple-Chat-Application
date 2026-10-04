#pragma once

// Shared constants for client and server.
// A "header" file is a description of names we want to reuse.
// We include it from more than one .cpp file so both programs agree
// on the same port, line size, and device path.

#include <string>

namespace chat {

// Default TCP port if the user does not pass one.
// A port is like a "room number" on this computer.
inline constexpr int kDefaultPort = 5000;

// Maximum length of one chat line (including newline).
inline constexpr int kMaxLine = 2048;

// How many history rows to show for /history.
inline constexpr int kHistoryLimit = 20;

// Linux character device used for server statistics.
// This file appears only after the kernel module is loaded.
inline constexpr const char* kMonitorDevice = "/dev/chat_monitor";

inline constexpr const char* kDbPath = "database/chat.db";

}  // namespace chat
