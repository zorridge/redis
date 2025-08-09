#include "epoll_event_loop.hpp"

#ifdef __linux__

#include <arpa/inet.h>
#include <iostream>
#include <list>
#include <sys/epoll.h>
#include <unistd.h>

void EpollEventLoop::run(SocketRAII &server_socket,
                         DataStore &store,
                         CommandDispatcher &dispatcher,
                         BlockingManager &blocking_manager,
                         std::atomic<bool> &running)
{
  // 1. Create a new epoll instance
  int event_queue_fd = epoll_create1(0);
  if (event_queue_fd == -1)
  {
    perror("epoll_create1 failed");
    return;
  }
  SocketRAII epoll_raii{event_queue_fd};

  // 2. Register the server socket for read events
  struct epoll_event event;
  event.events = EPOLLIN; // EPOLLIN signifies that the socket is ready for reading
  event.data.fd = server_socket.get();
  if (epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, server_socket.get(), &event) == -1)
  {
    perror("epoll_ctl server socket registration failed");
    return;
  }
  std::cout << "\n\033[35m[Server] Listening on localhost:6379 (epoll model)\033[0m\n";

  // Map to store state for each client connection
  std::map<int, ClientHandler> clients;

  struct epoll_event events[128];

  while (running)
  {
    // The timeout for epoll_wait is an integer in milliseconds.
    int timeout_ms = 1000; // 1 second

    // 3. Wait for events
    int num_events = epoll_wait(event_queue_fd, events, 128, timeout_ms);
    if (num_events == -1)
    {
      if (errno == EINTR)
        continue; // Interrupted by signal, loop again
      perror("epoll_wait failed");
      break;
    }

    // 4. Process events
    for (int i = 0; i < num_events; ++i)
    {
      int event_fd = events[i].data.fd;

      // With epoll, disconnection is often signaled by EPOLLHUP (hang-up) or EPOLLERR.
      if (events[i].events & (EPOLLHUP | EPOLLERR))
      {
        std::cout << "\033[33m[Client " << event_fd << "] Disconnected\033[0m\n";
        clients.erase(event_fd);
      }
      else if (event_fd == server_socket.get())
      {
        // read event on the server socket -> new connection
        int client_raw_fd = accept(server_socket.get(), nullptr, nullptr);
        if (client_raw_fd < 0)
        {
          perror("accept failed");
          continue;
        }

        // Add the new client to epoll to watch for read events
        event.events = EPOLLIN;
        event.data.fd = client_raw_fd;
        epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, client_raw_fd, &event);

        // Create a handler for this client
        clients.emplace(client_raw_fd, ClientHandler(client_raw_fd));
        std::cout << "\033[33m[Client " << client_raw_fd << "] Connected\033[0m\n";
      }
      else if (events[i].events & EPOLLIN)
      {
        // read event on a client socket -> data available
        auto it = clients.find(event_fd);
        if (it != clients.end())
        {
          it->second.handle_read(dispatcher);
        }
      }
    }

    // 5. Re-process clients that became ready
    auto &ready_clients = blocking_manager.get_ready_list();
    if (!ready_clients.empty())
    {
      for (int fd : ready_clients)
      {
        auto it = clients.find(fd);
        if (it != clients.end())
        {
          it->second.handle_reprocess(dispatcher);
        }
      }
      blocking_manager.clear_ready_list();
    }

    // 6. Process timed-out clients
    std::vector<int> timed_out_fds = blocking_manager.find_and_clear_timed_out_clients();
    for (int fd : timed_out_fds)
    {
      auto it = clients.find(fd);
      if (it != clients.end())
      {
        std::cout << "\033[33m[Client " << fd << "] Timed out\033[0m\n";
        const char *nil_reply = "*-1\r\n";
        send(fd, nil_reply, 5, 0);
      }
    }
  }
}

#endif // __linux__
