#pragma once

#include "event_loop.hpp"

#ifdef __linux__

class EpollEventLoop : public EventLoop
{
public:
  void run(SocketRAII &server_socket,
           DataStore &store,
           CommandDispatcher &dispatcher,
           std::atomic<bool> &running) override;
};

#endif // __linux__
