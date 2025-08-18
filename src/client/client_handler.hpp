#pragma once

#include "../resp/resp_parser.hpp"
#include "../socket/socket.hpp"

#include <optional>
#include <unordered_set>

class CommandDispatcher;

class ClientHandler
{
public:
  explicit ClientHandler(int client_fd);
  int get_fd() const { return m_client_fd.get(); };

  void handle_read(CommandDispatcher &dispatcher);
  void handle_reprocess(CommandDispatcher &dispatcher);

  // Transactions
  const std::vector<RESPValue> &get_command_queue() const { return m_command_queue; }
  bool is_in_transaction() const { return m_in_transaction; }
  void start_transaction();
  void queue_command(const RESPValue &command);
  void clear_transaction_state();

  // Pub/sub
  const std::unordered_set<std::string> &get_subscribed_channels() const { return m_subscribed_channels; }
  bool add_subscription(const std::string &channel);
  bool remove_subscription(const std::string &channel);
  size_t clear_subscriptions();
  bool is_subscriber_mode() const;

  // Pub/sub send helpers
  bool send_pubsub_message(const std::string &type, const std::string &channel, const RESPValue &payload);

  // Client I/O
  void queue_message(const std::string &msg);
  bool has_pending_output() const;
  bool flush_output();

private:
  SocketRAII m_client_fd;
  RESPParser m_parser;

  std::optional<RESPValue> m_blocked_command;

  bool m_in_transaction = false;
  std::vector<RESPValue> m_command_queue;

  std::string m_outgoing_buffer;
  std::unordered_set<std::string> m_subscribed_channels;

  static std::string to_upper(const std::string &s);
};
