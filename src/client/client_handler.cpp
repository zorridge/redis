#include "client_handler.hpp"
#include "../command/command_dispatcher.hpp"
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

  ssize_t bytes_received = recv(get_fd(), buffer, BUFFER_SIZE, 0);

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
    send(get_fd(), response.c_str(), response.size(), 0);
    return;
  }

  // Transaction logic
  std::string cmd_name = to_upper(value.array[0].str);
  if (m_in_transaction && cmd_name != "EXEC" && cmd_name != "DISCARD")
  {
    queue_command(value);
    std::string response = RESPSerializer::serialize(RESPValue::SimpleString("QUEUED"));
    send(m_client_fd.get(), response.c_str(), response.size(), 0);
    return;
  }

  RESPValue result = dispatcher.dispatch(value, *this);

  if (result.type == RESPValue::Type::Array &&
      result.array.size() == 2 &&
      result.array[0].type == RESP_BLOCK_CLIENT.type &&
      result.array[0].str == RESP_BLOCK_CLIENT.str)
  {
    std::cout << "\033[33m[Client " << get_fd() << "] Blocked\033[0m\n";
    m_blocked_command = std::move(result.array[1]);
    return;
  }

  std::string response = RESPSerializer::serialize(result);
  send(get_fd(), response.c_str(), response.size(), 0);
}

void ClientHandler::handle_reprocess(CommandDispatcher &dispatcher)
{
  if (!m_blocked_command.has_value())
    return;

  std::cout << "\033[33m[Client " << get_fd() << "] Reprocessing\033[0m\n";

  RESPValue value = std::move(m_blocked_command.value());
  m_blocked_command.reset();

  RESPValue result = dispatcher.dispatch(value, *this);
  if (result.type == RESP_BLOCK_CLIENT.type && result.str == RESP_BLOCK_CLIENT.str)
  {
    std::cout << "\033[33m[Client " << get_fd() << "] Re-blocked\033[0m\n";
    m_blocked_command = std::move(value);
    return;
  }

  std::string response = RESPSerializer::serialize(result);
  send(get_fd(), response.c_str(), response.size(), 0);
}

void ClientHandler::start_transaction()
{
  m_in_transaction = true;
  m_command_queue.clear();
}

void ClientHandler::queue_command(const RESPValue &command)
{
  m_command_queue.push_back(command);
}

void ClientHandler::clear_transaction_state()
{
  m_in_transaction = false;
  m_command_queue.clear();
}

std::string ClientHandler::to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}