#include "event_loop.hpp"

#ifdef __APPLE__
#include "kqueue_event_loop.hpp"
#elif __linux__
#include "epoll_event_loop.hpp"
#endif

EventLoop *EventLoop::create()
{
#ifdef __APPLE__
  return new KqueueEventLoop();
#elif __linux__
  return new EpollEventLoop();
#else
  return nullptr;
#endif
}
