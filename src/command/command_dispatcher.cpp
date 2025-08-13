#include "command_dispatcher.hpp"
#include "../client/client_handler.hpp"
#include "../utils/utils.hpp"

#include <algorithm>

CommandDispatcher::CommandDispatcher(DataStore &store, BlockingManager &blocking_manager, Config &config)
    : m_store(store), m_blocking_manager(blocking_manager), m_config(config) {}

void CommandDispatcher::register_command(const std::string &cmd, Handler handler)
{
  m_handlers[to_upper(cmd)] = std::move(handler);
}

RESPValue CommandDispatcher::dispatch(const RESPValue &value, ClientHandler &client) const
{
  printCommand(value);

  if (value.type != RESPValue::Type::Array ||
      value.array.empty() ||
      !std::all_of(value.array.begin(), value.array.end(),
                   [](const RESPValue &elem)
                   {
                     return elem.type == RESPValue::Type::BulkString;
                   }))
  {
    return RESPValue::Error("ERR Protocol error");
  }

  std::string cmd = to_upper(value.array[0].str);
  auto it = m_handlers.find(cmd);
  if (it != m_handlers.end())
  {
    return it->second(value, client);
  }

  return RESPValue::Error("ERR unknown command");
}

std::string CommandDispatcher::to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}
