#pragma once

#include "blocking_manager.hpp"
#include "../command/command_dispatcher.hpp"
#include "../client/client_handler.hpp"
#include "../pubsub/pubsub_manager.hpp"
#include "../socket/socket.hpp"
#include <map>
#include <memory>
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
      BlockingManager &blocking_manager,
      PubSubManager &pubsub_manager,
      std::atomic<bool> &running) = 0;

  // Factory function to create the correct loop for the current platform
  static std::unique_ptr<EventLoop> create();
};