#include "commands.hpp"

std::string to_upper(const std::string &s);

namespace commands
{
  RESPValue ping(const RESPValue &value)
  {
    if (value.array.size() == 1)
      return RESPValue::SimpleString("PONG");
    if (value.array.size() == 2)
      return value.array[1];
    return RESPValue::Error("wrong number of arguments for 'PING' command");
  }

  RESPValue command(const RESPValue &)
  {
    return RESPValue::Array({});
  }

  RESPValue echo(const RESPValue &value)
  {
    if (value.array.size() != 2)
      return RESPValue::Error("wrong number of arguments for 'ECHO' command");
    return value.array[1];
  }

  RESPValue set(const RESPValue &value, DataStore &store)
  {
    // SET key value [PX milliseconds]
    if (value.array.size() != 3 && value.array.size() != 5)
      return RESPValue::Error("wrong number of arguments for 'SET' command");

    if (value.array.size() == 3)
    {
      return store.set(value.array[1].str, value.array[2].str);
    }

    if (to_upper(value.array[3].str) != "PX")
      return RESPValue::Error("invalid request");

    try
    {
      std::chrono::milliseconds ttl{std::stoll(value.array[4].str)};
      if (ttl.count() <= 0)
        return RESPValue::Error("invalid PX value");
      return store.set(value.array[1].str, value.array[2].str, ttl);
    }
    catch (...)
    {
      return RESPValue::Error("invalid PX value");
    }
  }

  RESPValue get(const RESPValue &value, DataStore &store)
  {
    if (value.array.size() != 2)
      return RESPValue::Error("wrong number of arguments for 'GET' command");
    return store.get(value.array[1].str);
  }
}

std::string to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}