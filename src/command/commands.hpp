#pragma once

#include "../resp/resp_value.hpp"
#include "../data_store/data_store.hpp"

namespace commands
{
  RESPValue ping(const RESPValue &value);
  RESPValue command(const RESPValue &value);
  RESPValue echo(const RESPValue &value);
  RESPValue set(const RESPValue &value, DataStore &store);
  RESPValue get(const RESPValue &value, DataStore &store);
}
