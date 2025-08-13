#pragma once

#include "command_dispatcher.hpp"
#include "../event_loop/blocking_manager.hpp"
#include "../client/client_handler.hpp"
#include "../data_store/data_store.hpp"
#include "../resp/resp_value.hpp"

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher);

  RESPValue ping(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue command(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue echo(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);

  RESPValue type(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);

  // String
  RESPValue set(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue get(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue incr(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);

  // List
  RESPValue llen(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue rpush(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue lpush(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue lrange(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue lpop(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue blpop(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);

  // Stream
  RESPValue xadd(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue xrange(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue xread(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);

  // Transactions
  RESPValue multi(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue discard(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
  RESPValue exec(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);

  // Configs
  RESPValue config(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher);
}
