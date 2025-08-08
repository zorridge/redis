#include "command/command_dispatcher.hpp"
#include "command/commands.hpp"
#include "data_store/data_store.hpp"
#include "socket/socket.hpp"
#include "event_loop/event_loop.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>

// Global state for signal handling
std::atomic<bool> running{true};

void handle_signal(int)
{
  running = false;
}

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

  std::unique_ptr<EventLoop> event_loop(EventLoop::create());
  if (!event_loop)
  {
    std::cerr << "Failed to create event loop for this platform." << std::endl;
    return 1;
  }

  // Run the main event loop
  event_loop->run(server_socket, store, dispatcher, running);

  std::cout << "\033[35m[Server] Shutting down\033[0m\n";
  return 0;
}
