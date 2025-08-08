#pragma once
#include "event_loop.hpp"

#ifdef __APPLE__

class KqueueEventLoop : public EventLoop
{
public:
  void run(SocketRAII &server_socket,
           DataStore &store,
           CommandDispatcher &dispatcher,
           std::atomic<bool> &running) override;
};

#endif // __APPLE__