#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>

#include "command/register.hpp"
#include "command/command_dispatcher.hpp"
#include "data_store/data_store.hpp"
#include "handler/client_handler.hpp"
#include "socket/socket.hpp"

int g_server_fd = -1;
std::atomic<bool> running{true};

void handle_signal(int)
{
  running = false;
  if (g_server_fd >= 0)
    close(g_server_fd);
}

int main(int argc, char *argv[])
{
  std::signal(SIGINT, handle_signal);
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  g_server_fd = set_up();
  if (g_server_fd < 0)
    return -1;
  SocketRAII server_fd{g_server_fd};

  std::cout << "\n\033[35m[Server] Listening on localhost:6379\033[0m\n";

  DataStore store;
  CommandDispatcher dispatcher(store);
  register_all_commands(dispatcher);

  std::vector<std::thread> threads;
  while (running)
  {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    int raw_client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_addr_len);
    if (raw_client_fd < 0)
    {
      std::cerr << "Accept failed\n";
      continue;
    }

    SocketRAII client_fd{raw_client_fd};
    threads.emplace_back([fd = std::move(client_fd), &dispatcher]() mutable
                         { handle_client(std::move(fd), dispatcher); });
  }

  for (auto &t : threads)
  {
    if (t.joinable())
      t.join();
  }

  std::cout << "\033[35m[Server] Shutting down\033[0m\n";
  return 0;
}

/*
1. ✅ TCP Server Layer
2. ✅ RESP Parser
3. ✅ Command Dispatcher
4. ✅ Data Store
5. ✅ RESP Serializer
*/