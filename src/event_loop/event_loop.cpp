#include "event_loop.hpp"

#ifdef __APPLE__
#include "kqueue_event_loop.hpp"
#elif __linux__
#include "epoll_event_loop.hpp"
#endif

std::unique_ptr<EventLoop> EventLoop::create()
{
#ifdef __APPLE__
  return std::make_unique<KqueueEventLoop>();
#elif __linux__
  return std::make_unique<EpollEventLoop>();
#else
  return nullptr;
#endif
}
