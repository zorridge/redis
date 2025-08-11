#pragma once

#include "command_dispatcher.hpp"
#include "../event_loop/blocking_manager.hpp"
#include "../client/client_handler.hpp"
#include "../data_store/data_store.hpp"
#include "../resp/resp_value.hpp"

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher);

  RESPValue ping(const RESPValue &value);
  RESPValue command(const RESPValue &value);
  RESPValue echo(const RESPValue &value);

  RESPValue type(const RESPValue &value, DataStore &store);

  // String
  RESPValue set(const RESPValue &value, DataStore &store);
  RESPValue get(const RESPValue &value, DataStore &store);
  RESPValue incr(const RESPValue &value, DataStore &store);

  // List
  RESPValue llen(const RESPValue &value, DataStore &store);
  RESPValue rpush(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager);
  RESPValue lpush(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager);
  RESPValue lrange(const RESPValue &value, DataStore &store);
  RESPValue lpop(const RESPValue &value, DataStore &store);
  RESPValue blpop(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager, int client_fd);

  // Stream
  RESPValue xadd(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager);
  RESPValue xrange(const RESPValue &value, DataStore &store);
  RESPValue xread(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager, int client_fd);

  // Transactions
  RESPValue multi(const RESPValue &, ClientHandler &);
  RESPValue exec(const RESPValue &, ClientHandler &, CommandDispatcher &);
  RESPValue discard(const RESPValue &, ClientHandler &);
}
