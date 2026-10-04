CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Iinclude -Iserver -Iclient
LDFLAGS  ?= -pthread -lsqlite3

SERVER_SRC = \
	server/main.cpp \
	server/ChatServer.cpp \
	server/ClientHandler.cpp \
	server/Authentication.cpp \
	server/MessageManager.cpp \
	server/RoomManager.cpp \
	server/Database.cpp

CLIENT_SRC = \
	client/main.cpp \
	client/ChatClient.cpp

.PHONY: all clean test server client

all: bin/chat_server bin/chat_client

bin:
	mkdir -p bin

bin/chat_server: $(SERVER_SRC) include/common.hpp include/sha256.hpp | bin
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $@ $(LDFLAGS)

bin/chat_client: $(CLIENT_SRC) include/common.hpp | bin
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $@ -pthread

test: bin/test_auth bin/test_database bin/test_message
	./bin/test_auth
	./bin/test_database
	./bin/test_message

bin/test_auth: tests/test_auth.cpp server/Authentication.cpp server/Database.cpp | bin
	$(CXX) $(CXXFLAGS) tests/test_auth.cpp server/Authentication.cpp server/Database.cpp -o $@ $(LDFLAGS)

bin/test_database: tests/test_database.cpp server/Database.cpp | bin
	$(CXX) $(CXXFLAGS) tests/test_database.cpp server/Database.cpp -o $@ $(LDFLAGS)

bin/test_message: tests/test_message.cpp server/MessageManager.cpp server/Database.cpp | bin
	$(CXX) $(CXXFLAGS) tests/test_message.cpp server/MessageManager.cpp server/Database.cpp -o $@ $(LDFLAGS)

clean:
	rm -rf bin
	rm -f database/chat.db database/chat.db-journal tests/*.tmp.db

.PHONY: all clean test
