#include <algorithm>

#include "command_dispatcher.hpp"
#include "../resp/resp_serializer.hpp"

CommandDispatcher::CommandDispatcher(DataStore &store)
    : m_store(store) {}

void CommandDispatcher::register_command(const std::string &cmd, Handler handler)
{
  m_handlers[to_upper(cmd)] = std::move(handler);
}

std::string CommandDispatcher::dispatch(const RESPValue &value) const
{
  if (value.type != RESPValue::Type::Array ||
      value.array.empty() ||
      !std::all_of(value.array.begin(), value.array.end(),
                   [](const RESPValue &elem)
                   {
                     return elem.type == RESPValue::Type::BulkString;
                   }))
  {
    return RESPSerializer::serialize(RESPValue::Error("invalid request"));
  }

  std::string cmd = to_upper(value.array[0].str);
  auto it = m_handlers.find(cmd);
  if (it != m_handlers.end())
  {
    return RESPSerializer::serialize(it->second(value));
  }

  return RESPSerializer::serialize(RESPValue::Error("unknown command"));
}

std::string CommandDispatcher::to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}
