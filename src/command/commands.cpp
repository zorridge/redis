#include "commands.hpp"

std::string to_upper(const std::string &s);

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher)
  {
    dispatcher.register_command("PING", commands::ping);
    dispatcher.register_command("COMMAND", commands::command);
    dispatcher.register_command("ECHO", commands::echo);

    dispatcher.register_command("SET", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::set(value, dispatcher.get_store()); });
    dispatcher.register_command("GET", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::get(value, dispatcher.get_store()); });

    dispatcher.register_command("LLEN", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return llen(value, dispatcher.get_store()); });
    dispatcher.register_command("RPUSH", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::rpush(value, dispatcher.get_store()); });
    dispatcher.register_command("LPUSH", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return lpush(value, dispatcher.get_store()); });
    dispatcher.register_command("LRANGE", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::lrange(value, dispatcher.get_store()); });
    dispatcher.register_command("LPOP", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return lpop(value, dispatcher.get_store()); });
    dispatcher.register_command("BLPOP", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return blpop(value, dispatcher.get_store()); });
  }

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

  RESPValue llen(const RESPValue &value, DataStore &store)
  {
    // LLEN key
    if (value.array.size() != 2)
      return RESPValue::Error("wrong number of arguments for 'LLEN' command");

    const std::string &key = value.array[1].str;
    return store.llen(key);
  }

  RESPValue rpush(const RESPValue &value, DataStore &store)
  {
    // RPUSH key element [element ...]
    if (value.array.size() < 3)
      return RESPValue::Error("wrong number of arguments for 'RPUSH' command");

    const std::string &key = value.array[1].str;
    std::vector<std::string> values;
    for (size_t i = 2; i < value.array.size(); ++i)
      values.push_back(value.array[i].str);

    return store.rpush(key, values);
  }

  RESPValue lpush(const RESPValue &value, DataStore &store)
  {
    // LPUSH key element [element ...]
    if (value.array.size() < 3)
      return RESPValue::Error("wrong number of arguments for 'LPUSH' command");

    const std::string &key = value.array[1].str;
    std::vector<std::string> values;
    for (size_t i = 2; i < value.array.size(); ++i)
      values.push_back(value.array[i].str);

    return store.lpush(key, values);
  }

  RESPValue lrange(const RESPValue &value, DataStore &store)
  {
    // LRANGE key start stop
    if (value.array.size() != 4)
      return RESPValue::Error("wrong number of arguments for 'LRANGE' command");

    const std::string &key = value.array[1].str;
    try
    {
      int64_t start = std::stoll(value.array[2].str);
      int64_t stop = std::stoll(value.array[3].str);
      return store.lrange(key, start, stop);
    }
    catch (...)
    {
      return RESPValue::Error("start or stop is not an integer");
    }
  }

  RESPValue lpop(const RESPValue &value, DataStore &store)
  {
    // LPOP key [count]
    if (value.array.size() < 2 || value.array.size() > 3)
      return RESPValue::Error("wrong number of arguments for 'LPOP' command");

    const std::string &key = value.array[1].str;
    int64_t count = 1;
    if (value.array.size() == 3)
    {
      try
      {
        count = std::stoll(value.array[2].str);
      }
      catch (...)
      {
        return RESPValue::Error("value is not an integer");
      }

      if (count < 0)
        return RESPValue::Error("value is out of range, must be positive");
      if (count == 0)
        return RESPValue::Array({});
    }

    return store.lpop(key, count);
  }

  RESPValue blpop(const RESPValue &value, DataStore &store)
  {
    // BLPOP key timeout
    if (value.array.size() != 3)
      return RESPValue::Error("wrong number of arguments for 'BLPOP' command");

    const std::string &key = value.array[1].str;
    double timeout = 0.0;
    try
    {
      timeout = std::stod(value.array[2].str);
      if (timeout < 0.0)
        return RESPValue::Error("timeout is negative");
    }
    catch (...)
    {
      return RESPValue::Error("timeout is not a valid double");
    }

    return store.blpop(key, timeout);
  }
}

std::string to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}