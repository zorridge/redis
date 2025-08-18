#include "kqueue_event_loop.hpp"

#ifdef __APPLE__

#include <arpa/inet.h>
#include <iostream>
#include <list>
#include <sys/event.h>
#include <unistd.h>

// Helper to add/modify events in kqueue
static void update_kqueue_event(int kq, int ident, int16_t filter, uint16_t flags)
{
  struct kevent change;
  EV_SET(&change, ident, filter, flags, 0, 0, nullptr);
  if (kevent(kq, &change, 1, nullptr, 0, nullptr) == -1)
  {
    perror("kevent update failed");
  }
}

void KqueueEventLoop::run(SocketRAII &server_socket,
                          DataStore &store,
                          CommandDispatcher &dispatcher,
                          BlockingManager &blocking_manager,
                          PubSubManager &pubsub_manager,
                          std::atomic<bool> &running)
{
  // 1. Create a new kqueue
  int event_queue_fd = kqueue();
  if (event_queue_fd == -1)
  {
    perror("kqueue failed");
    return;
  }
  SocketRAII kqueue_raii{event_queue_fd};

  // 2. Register the server socket
  update_kqueue_event(event_queue_fd, server_socket.get(), EVFILT_READ, EV_ADD | EV_ENABLE);
  std::cout << "\n\033[35m[Server] Listening on localhost:6379 (kqueue model)\033[0m\n";

  // Map to store state for each client connection
  std::map<int, ClientHandler> clients;

  struct kevent events[128];

  while (running)
  {
    // Timeout allows the loop to wake up periodically to check for client timeouts, even if there is no network activity.
    struct timespec timeout = {1, 0}; // 1 second, 0 nanoseconds

    // 3. Wait for events
    int num_events = kevent(event_queue_fd, nullptr, 0, events, 128, &timeout);
    if (num_events == -1)
    {
      if (errno == EINTR)
        continue; // Interrupted by signal, loop again
      perror("kevent wait failed");
      break;
    }

    // 4. Process events
    for (int i = 0; i < num_events; ++i)
    {
      int event_fd = events[i].ident;

      if (events[i].flags & EV_EOF)
      {
        std::cout << "\033[33m[Client " << event_fd << "] Disconnected\033[0m\n";

        auto it = clients.find(event_fd);
        if (it != clients.end())
        {
          dispatcher.get_pubsub_manager().unsubscribe_all(&it->second);
          blocking_manager.unblock_client(event_fd);

          // Remove from kqueue
          update_kqueue_event(event_queue_fd, event_fd, EVFILT_READ, EV_DELETE);

          clients.erase(it); // RAII closes socket
        }
      }
      else if (event_fd == server_socket.get())
      {
        // read event on the server (listening) socket -> new connection
        int client_raw_fd = accept(server_socket.get(), nullptr, nullptr);
        if (client_raw_fd < 0)
        {
          perror("accept failed");
          continue;
        }

        // Add the new client to kqueue to watch for read events
        update_kqueue_event(event_queue_fd, client_raw_fd, EVFILT_READ, EV_ADD | EV_ENABLE);

        // Create a handler for this client
        clients.emplace(client_raw_fd, ClientHandler(client_raw_fd));
        std::cout << "\033[33m[Client " << client_raw_fd << "] Connected\033[0m\n";
      }
      else if (events[i].filter == EVFILT_READ)
      {
        // read event on a client (connected) socket -> data available
        auto it = clients.find(event_fd);
        if (it != clients.end())
          it->second.handle_read(dispatcher);
      }
      else if (events[i].filter == EVFILT_WRITE)
      {
        auto it = clients.find(event_fd);
        if (it != clients.end())
        {
          bool sent_all = it->second.flush_output();
          if (sent_all)
            // Buffer is empty, disable EVFILT_WRITE for this client
            update_kqueue_event(event_queue_fd, event_fd, EVFILT_WRITE, EV_DELETE);
          // else keep writable event enabled
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
          it->second.handle_reprocess(dispatcher);
      }
      blocking_manager.clear_ready_list();
    }

    // 6. Process timed-out clients
    std::vector<int> timed_out_fds = blocking_manager.find_and_clear_timed_out_clients();
    if (!timed_out_fds.empty())
    {
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

    // 7. Sweep all clients for pending output after reads/reprocess/timed-out
    for (auto &[fd, client] : clients)
    {
      if (client.has_pending_output())
        // Mark client as writable
        update_kqueue_event(event_queue_fd, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE);
    }
  }
}

#endif // __APPLE__
