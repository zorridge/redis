#pragma once

#include "blocking_manager.hpp"
#include "event_loop.hpp"

#ifdef __linux__

class EpollEventLoop : public EventLoop
{
public:
  void run(SocketRAII &server_socket,
           DataStore &store,
           CommandDispatcher &dispatcher,
           BlockingManager &blocking_manager,
           PubSubManager &pubsub_manager,
           std::atomic<bool> &running) override;
};

#endif // __linux__
