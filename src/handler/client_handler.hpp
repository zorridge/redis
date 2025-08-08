#pragma once

#include "../command/command_dispatcher.hpp"
#include "../resp/resp_parser.hpp"
#include "../socket/socket.hpp"

class ClientHandler
{
public:
  explicit ClientHandler(int client_fd);

  void handle_read(CommandDispatcher &dispatcher);

private:
  SocketRAII m_client_fd;
  RESPParser m_parser;
};
