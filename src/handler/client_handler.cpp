#include "client_handler.hpp"
#include "../utils/utils.hpp"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

ClientHandler::ClientHandler(int client_fd) : m_client_fd(client_fd) {}

void ClientHandler::handle_read(CommandDispatcher &dispatcher)
{
  constexpr size_t BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];

  ssize_t bytes_received = recv(m_client_fd.get(), buffer, BUFFER_SIZE, 0);

  // Disconnection is handled by EV_EOF in the main event loop
  // 0 bytes received also indicates disconnection
  // So we will simply return and let main handle the clean up
  if (bytes_received <= 0)
    return;

  m_parser.feed(buffer, bytes_received);
  RESPValue value = m_parser.parse();
  if (value.type == RESPValue::Type::Error)
  {
    std::string response = "-" + value.str + "\r\n";
    send(m_client_fd.get(), response.c_str(), response.size(), 0);
  }
  else
  {
    printCommand(value);

    std::string response = dispatcher.dispatch(value);
    send(m_client_fd.get(), response.c_str(), response.size(), 0);
  }
}
