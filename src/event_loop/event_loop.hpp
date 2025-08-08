#pragma once

#include "../command/command_dispatcher.hpp"
#include "../handler/client_handler.hpp"
#include "../socket/socket.hpp"
#include <map>
#include <atomic>

class EventLoop
{
public:
  virtual ~EventLoop() = default;

  // Main function
  virtual void run(
      SocketRAII &server_socket,
      DataStore &store,
      CommandDispatcher &dispatcher,
      std::atomic<bool> &running) = 0;

  // Factory function to create the correct loop for the current platform
  static EventLoop *create();
};