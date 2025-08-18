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
    queue_message(response);
    return;
  }

  std::string cmd_name = to_upper(value.array[0].str);

  // ---------- Pub/sub ----------
  if (is_subscriber_mode())
  {
    static const std::unordered_set<std::string> allowed = {"SUBSCRIBE", "UNSUBSCRIBE", "PSUBSCRIBE", "PUNSUBSCRIBE", "PING", "QUIT"};
    if (allowed.find(cmd_name) == allowed.end())
    {
      queue_message(RESPSerializer::serialize(RESPValue::Error("ERR Can't execute '" + cmd_name + "' in subscribed mode")));
      return;
    }
  }

  // ---------- Transactions ----------
  if (m_in_transaction && cmd_name != "EXEC" && cmd_name != "DISCARD")
  {
    queue_command(value);
    queue_message(RESPSerializer::serialize(RESPValue::SimpleString("QUEUED")));
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

  queue_message(RESPSerializer::serialize(result));
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

  queue_message(RESPSerializer::serialize(result));
}

// ---------- Transactions ----------
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

// ---------- Pub/sub ----------
bool ClientHandler::add_subscription(const std::string &channel)
{
  auto [it, inserted] = m_subscribed_channels.insert(channel);
  return inserted;
}

bool ClientHandler::remove_subscription(const std::string &channel)
{
  return m_subscribed_channels.erase(channel) > 0;
}

size_t ClientHandler::clear_subscriptions()
{
  size_t n = m_subscribed_channels.size();
  m_subscribed_channels.clear();
  return n;
}

bool ClientHandler::is_subscriber_mode() const
{
  return !m_subscribed_channels.empty();
}

// ---------- Pub/Sub send helpers ----------
bool ClientHandler::send_pubsub_message(const std::string &type, const std::string &channel, const RESPValue &payload)
{
  RESPValue msg = RESPValue::Array({RESPValue::BulkString(type), RESPValue::BulkString(channel), payload});
  std::string response = RESPSerializer::serialize(msg);

  queue_message(response);
  return true;
}

void ClientHandler::queue_message(const std::string &msg)
{
  m_outgoing_buffer += msg;
}

bool ClientHandler::has_pending_output() const
{
  return !m_outgoing_buffer.empty();
}

bool ClientHandler::flush_output()
{
  while (!m_outgoing_buffer.empty())
  {
    ssize_t n = ::send(get_fd(), m_outgoing_buffer.data(), m_outgoing_buffer.size(), 0);
    if (n > 0)
      m_outgoing_buffer.erase(0, n);
    else
      return false;
  }
  return true;
}

std::string ClientHandler::to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}