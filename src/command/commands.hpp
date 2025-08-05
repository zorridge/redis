#pragma once

#include "command_dispatcher.hpp"
#include "../resp/resp_value.hpp"
#include "../data_store/data_store.hpp"

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher);

  RESPValue ping(const RESPValue &value);
  RESPValue command(const RESPValue &value);
  RESPValue echo(const RESPValue &value);

  RESPValue set(const RESPValue &value, DataStore &store);
  RESPValue get(const RESPValue &value, DataStore &store);

  RESPValue llen(const RESPValue &value, DataStore &store);
  RESPValue rpush(const RESPValue &value, DataStore &store);
  RESPValue lpush(const RESPValue &value, DataStore &store);
  RESPValue lrange(const RESPValue &value, DataStore &store);
  RESPValue lpop(const RESPValue &value, DataStore &store);
  RESPValue blpop(const RESPValue &value, DataStore &store);
}
