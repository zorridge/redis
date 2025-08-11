#pragma once

#include "../resp/resp_parser.hpp"
#include "../socket/socket.hpp"

#include <optional>

class CommandDispatcher;

class ClientHandler
{
public:
  explicit ClientHandler(int client_fd);
  int get_fd() const { return m_client_fd.get(); };

  void handle_read(CommandDispatcher &dispatcher);
  void handle_reprocess(CommandDispatcher &dispatcher);

  const std::vector<RESPValue> &get_command_queue() const { return m_command_queue; }
  bool is_in_transaction() const { return m_in_transaction; }
  void start_transaction();
  void queue_command(const RESPValue &command);
  void clear_transaction_state();

private:
  SocketRAII m_client_fd;
  RESPParser m_parser;

  std::optional<RESPValue> m_blocked_command;

  bool m_in_transaction = false;
  std::vector<RESPValue> m_command_queue;

  static std::string to_upper(const std::string &s);
};
