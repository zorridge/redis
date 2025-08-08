#include "command/command_dispatcher.hpp"
#include "command/commands.hpp"
#include "data_store/data_store.hpp"
#include "handler/client_handler.hpp"
#include "socket/socket.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#ifdef __APPLE__
#include <sys/event.h> // kqueue
#elif __linux__
#include <sys/epoll.h> // epoll
#endif

// Global state for signal handling
std::atomic<bool> running{true};

void handle_signal(int)
{
  running = false;
}

#ifdef __APPLE__
// Helper to add/modify events in kqueue
void update_kqueue_event(int kq, int ident, int16_t filter, uint16_t flags)
{
  struct kevent change;
  EV_SET(&change, ident, filter, flags, 0, 0, nullptr);
  if (kevent(kq, &change, 1, nullptr, 0, nullptr) == -1)
  {
    perror("kevent update failed");
  }
}
#endif

int main(int argc, char *argv[])
{
  std::signal(SIGINT, handle_signal);
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = set_up();
  if (server_fd < 0)
    return -1;
  SocketRAII server_socket{server_fd};

  DataStore store;
  CommandDispatcher dispatcher(store);
  commands::register_all_commands(dispatcher);

  // Map to store state for each client connection
  std::map<int, ClientHandler> clients;

// --- kqueue implementation ---
#ifdef __APPLE__
  // 1. Create a new kqueue
  int event_queue_fd = kqueue();
  if (event_queue_fd == -1)
  {
    perror("kqueue failed");
    return -1;
  }
  SocketRAII kqueue_raii{event_queue_fd};

  // 2. Register the server socket to listen for new connections (read events)
  update_kqueue_event(event_queue_fd, server_socket.get(), EVFILT_READ, EV_ADD | EV_ENABLE);

  std::cout << "\n\033[35m[Server] Listening on localhost:6379 (kqueue model)\033[0m\n";

  // Main event loop
  while (running)
  {
    struct kevent events[128];
    // 3. Wait for events and blocks efficiently
    int num_events = kevent(event_queue_fd, nullptr, 0, events, 128, nullptr);
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
        // Client disconnected
        std::cout << "\033[33m[Client " << event_fd << "] disconnected\033[0m\n";
        clients.erase(event_fd); // RAII will close the socket via the ClientHandler
      }
      else if (event_fd == server_socket.get())
      {
        // READ event on the server (listening) socket -> new connection
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
        // READ event on a client (connected) socket -> data available
        auto it = clients.find(event_fd);
        if (it != clients.end())
          it->second.handle_read(dispatcher);
      }
    }
  }

// --- epoll implementation ---
#elif __linux__
  // 1. Create a new epoll instance
  int event_queue_fd = epoll_create1(0);
  if (event_queue_fd == -1)
  {
    perror("epoll_create1 failed");
    return -1;
  }
  SocketRAII epoll_raii{event_queue_fd};

  // 2. Register the server socket for read events
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = server_socket.get();
  if (epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, server_socket.get(), &event) == -1)
  {
    perror("epoll_ctl server socket registration failed");
    return -1;
  }

  std::cout << "\n\033[35m[Server] Listening on localhost:6379 (epoll model)\033[0m\n";

  struct epoll_event events[128];
  while (running)
  {
    // 3. Wait for events
    int num_events = epoll_wait(event_queue_fd, events, 128, -1);
    if (num_events == -1)
    {
      if (errno == EINTR)
        continue;
      perror("epoll_wait failed");
      break;
    }

    // 4. Process events
    for (int i = 0; i < num_events; ++i)
    {
      int event_fd = events[i].data.fd;

      if (events[i].events & (EPOLLHUP | EPOLLERR))
      {
        // Client disconnected (Hang-up or Error)
        std::cout << "\033[33m[Client " << event_fd << "] Disconnected (HUP/ERR)\033[0m\n";
        clients.erase(event_fd);
      }
      else if (event_fd == server_socket.get())
      {
        // READ event on the server socket -> new connection
        int client_raw_fd = accept(server_socket.get(), nullptr, nullptr);
        if (client_raw_fd < 0)
          continue;

        event.events = EPOLLIN;
        event.data.fd = client_raw_fd;
        epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, client_raw_fd, &event);

        clients.emplace(client_raw_fd, ClientHandler(client_raw_fd));
        std::cout << "\033[33m[Client " << client_raw_fd << "] Connected\033[0m\n";
      }
      else if (events[i].events & EPOLLIN)
      {
        // READ event on a client socket -> data available
        auto it = clients.find(event_fd);
        if (it != clients.end())
        {
          it->second.handle_read(dispatcher);
        }
      }
    }
  }
#else
#error "Unsupported platform: No kqueue or epoll available"
#endif

  std::cout << "\033[35m[Server] Shutting down\033[0m\n";
  return 0;
}
