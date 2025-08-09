#include "client_handler.hpp"
#include "../resp/resp_serializer.hpp"
#include "../data_store/data_store.hpp"

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
    return;
  }

  RESPValue result = dispatcher.dispatch(value, m_client_fd.get());

  if (result.type == RESPValue::Type::Array &&
      result.array.size() == 2 &&
      result.array[0].type == RESP_BLOCK_CLIENT.type &&
      result.array[0].str == RESP_BLOCK_CLIENT.str)
  {
    std::cout << "\033[33m[Client " << m_client_fd.get() << "] Blocked\033[0m\n";
    m_blocked_command = std::move(result.array[1]);
    return;
  }

  std::string response = RESPSerializer::serialize(result);
  send(m_client_fd.get(), response.c_str(), response.size(), 0);
}

void ClientHandler::handle_reprocess(CommandDispatcher &dispatcher)
{
  if (!m_blocked_command.has_value())
    return;

  std::cout << "\033[33m[Client " << m_client_fd.get() << "] Reprocessing\033[0m\n";

  RESPValue value = std::move(m_blocked_command.value());
  m_blocked_command.reset();

  RESPValue result = dispatcher.dispatch(value, m_client_fd.get());
  if (result.type == RESP_BLOCK_CLIENT.type && result.str == RESP_BLOCK_CLIENT.str)
  {
    std::cout << "\033[33m[Client " << m_client_fd.get() << "] Re-blocked\033[0m\n";
    m_blocked_command = std::move(value);
    return;
  }

  std::string response = RESPSerializer::serialize(result);
  send(m_client_fd.get(), response.c_str(), response.size(), 0);
}
