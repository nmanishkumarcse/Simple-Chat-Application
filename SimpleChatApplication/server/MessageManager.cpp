#include "MessageManager.h"

MessageManager::MessageManager(Database& db) : db_(db) {}

bool MessageManager::store(const std::string& sender,
                           const std::string& receiver,
                           const std::string& room,
                           const std::string& text) {
    return db_.save_message(sender, receiver, room, text);
}

std::vector<MessageRow> MessageManager::history(const std::string& username, int limit) {
    return db_.recent_history(username, limit);
}
