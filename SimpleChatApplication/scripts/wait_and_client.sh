#!/bin/bash
cd /home/manish/Projects/SimpleChatApplication

echo "Waiting for chat server on port 5000..."
for i in $(seq 1 120); do
  if [ -x ./bin/chat_client ] && (ss -ltn 2>/dev/null | grep -q ':5000 ') || (netstat -ltn 2>/dev/null | grep -q ':5000 '); then
    echo "Connecting..."
    exec ./bin/chat_client 127.0.0.1 5000
  fi
  sleep 1
done

echo "Server did not start in time."
echo "If install is still asking for a password, finish that first."
read -r
