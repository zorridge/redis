#include "commands.hpp"

std::string to_upper(const std::string &s);

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher)
  {
    dispatcher.register_command("PING", commands::ping);
    dispatcher.register_command("COMMAND", commands::command);
    dispatcher.register_command("ECHO", commands::echo);

    dispatcher.register_command("TYPE", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::type(value, dispatcher.get_store()); });

    dispatcher.register_command("SET", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::set(value, dispatcher.get_store()); });
    dispatcher.register_command("GET", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::get(value, dispatcher.get_store()); });

    dispatcher.register_command("LLEN", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::llen(value, dispatcher.get_store()); });
    dispatcher.register_command("RPUSH", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::rpush(value, dispatcher.get_store()); });
    dispatcher.register_command("LPUSH", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::lpush(value, dispatcher.get_store()); });
    dispatcher.register_command("LRANGE", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::lrange(value, dispatcher.get_store()); });
    dispatcher.register_command("LPOP", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::lpop(value, dispatcher.get_store()); });
    dispatcher.register_command("BLPOP", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::blpop(value, dispatcher.get_store()); });

    dispatcher.register_command("XADD", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::xadd(value, dispatcher.get_store()); });
    dispatcher.register_command("XRANGE", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::xrange(value, dispatcher.get_store()); });
    dispatcher.register_command("XREAD", [&dispatcher](const RESPValue &value) -> RESPValue
                                { return commands::xread(value, dispatcher.get_store()); });
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

  RESPValue type(const RESPValue &value, DataStore &store)
  {
    // TYPE key
    if (value.array.size() != 2)
      return RESPValue::Error("wrong number of arguments for 'TYPE' command");
    return store.type(value.array[1].str);
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
      return RESPValue::Error("syntax error");

    try
    {
      std::chrono::milliseconds ttl{std::stoll(value.array[4].str)};
      if (ttl.count() <= 0)
        return RESPValue::Error("value must be positive");
      return store.set(value.array[1].str, value.array[2].str, ttl);
    }
    catch (...)
    {
      return RESPValue::Error("value is not an integer");
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
        return RESPValue::Error("value must be positive");
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
      return RESPValue::Error("timeout is not a double");
    }

    return store.blpop(key, timeout);
  }

  RESPValue xadd(const RESPValue &value, DataStore &store)
  {
    // XADD key id field1 value1 [field2 value2 ...]
    if (value.array.size() < 5 || (value.array.size() - 3) % 2 != 0)
      return RESPValue::Error("wrong number of arguments for 'XADD' command");

    StreamEntry entry;
    for (size_t i = 3; i < value.array.size(); i++)
      entry.push_back(value.array[i].str);

    return store.xadd(value.array[1].str, value.array[2].str, entry);
  }

  RESPValue xrange(const RESPValue &value, DataStore &store)
  {
    // XRANGE key start end [COUNT count]
    if (value.array.size() < 4)
      return RESPValue::Error("wrong number of arguments for 'XRANGE' command");

    int64_t count = -1;
    if (value.array.size() > 4)
    {
      if (value.array.size() != 6 || value.array[4].str != "COUNT")
        return RESPValue::Error("syntax error");

      try
      {
        count = std::stoll(value.array[5].str);
      }
      catch (...)
      {
        return RESPValue::Error("value is not an integer");
      }
    }

    return store.xrange(value.array[1].str, value.array[2].str, value.array[3].str, count);
  }

  RESPValue xread(const RESPValue &value, DataStore &store)
  {
    // Basic: XREAD STREAMS key id
    // Full: XREAD [COUNT count] [BLOCK ms] STREAMS key [key ...] id [id ...]
    size_t streams_pos = 0;
    for (size_t i = 1; i < value.array.size(); ++i)
    {
      if (to_upper(value.array[i].str) == "STREAMS")
      {
        streams_pos = i;
        break;
      }
    }

    if (streams_pos == 0)
      return RESPValue::Error("must be called with the STREAMS keyword");

    size_t args_after_streams = value.array.size() - (streams_pos + 1);
    if (args_after_streams == 0 || args_after_streams % 2 != 0)
      return RESPValue::Error("keys and IDs must be provided in pairs");

    size_t num_streams = args_after_streams / 2;
    std::vector<std::string> keys;
    std::vector<std::string> ids;
    keys.reserve(num_streams);
    ids.reserve(num_streams);

    // The first half are keys, the second half are IDs
    for (size_t i = 0; i < num_streams; ++i)
    {
      keys.push_back(value.array[streams_pos + 1 + i].str);
      ids.push_back(value.array[streams_pos + 1 + num_streams + i].str);
    }

    return store.xread(keys, ids);
  }
}

std::string to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}