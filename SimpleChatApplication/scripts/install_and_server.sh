#!/bin/bash
set -e
cd /home/manish/Projects/SimpleChatApplication

echo "========================================"
echo "Installing C++ / build tools"
echo "Enter your Ubuntu password if asked."
echo "========================================"

sudo apt-get update
sudo apt-get install -y build-essential g++ gcc make cmake git sqlite3 libsqlite3-dev pkg-config

echo
echo "Compiler:"
g++ --version | head -1

echo
echo "Building chat application..."
make

echo
echo "Starting chat server on port 5000"
echo "Leave this window open. Open clients in other terminals:"
echo "  ./bin/chat_client 127.0.0.1 5000"
echo "========================================"

exec ./bin/chat_server 5000
