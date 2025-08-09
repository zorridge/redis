#pragma once

#include "blocking_manager.hpp"
#include "event_loop.hpp"

#ifdef __APPLE__

class KqueueEventLoop : public EventLoop
{
public:
  void run(SocketRAII &server_socket,
           DataStore &store,
           CommandDispatcher &dispatcher,
           BlockingManager &blocking_manager,
           std::atomic<bool> &running) override;
};

#endif // __APPLE__