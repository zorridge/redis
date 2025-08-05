#include "client_handler.hpp"
#include "../resp/resp_parser.hpp"
#include "../utils/utils.hpp"

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

void handle_client(SocketRAII client_fd, CommandDispatcher &dispatcher)
{
  constexpr size_t BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];

  RESPParser parser;

  std::cout << "\033[33m[Client] Connected\033[0m\n";
  while (true)
  {
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0)
      break;

    parser.feed(buffer, bytes_received);
    RESPValue value = parser.parse();

    if (value.type == RESPValue::Type::Error)
    {
      std::string response = "-" + value.str + "\r\n";
      send(client_fd, response.c_str(), response.size(), 0);
      break;
    }

    printCommand(value);

    std::string response = dispatcher.dispatch(value);
    send(client_fd, response.c_str(), response.size(), 0);
  }

  std::cout << "\033[33m[Client] Exiting\033[0m\n";
}
