#include "commands.hpp"

std::string to_upper(const std::string &s);

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher)
  {
    auto adapt = [&dispatcher](auto func)
    {
      return [func, &dispatcher](const RESPValue &value, int client_fd) -> RESPValue
      {
        if constexpr (std::is_invocable_v<decltype(func), const RESPValue &>)
          return func(value);
        else if constexpr (std::is_invocable_v<decltype(func), const RESPValue &, DataStore &>)
          return func(value, dispatcher.get_store());
        else if constexpr (std::is_invocable_v<decltype(func), const RESPValue &, DataStore &, BlockingManager &>)
          return func(value, dispatcher.get_store(), dispatcher.get_blocking_manager());
        else if constexpr (std::is_invocable_v<decltype(func), const RESPValue &, DataStore &, BlockingManager &, int>)
          return func(value, dispatcher.get_store(), dispatcher.get_blocking_manager(), client_fd);
      };
    };

    dispatcher.register_command("PING", adapt(commands::ping));
    dispatcher.register_command("COMMAND", adapt(commands::command));
    dispatcher.register_command("ECHO", adapt(commands::echo));

    dispatcher.register_command("TYPE", adapt(commands::type));

    dispatcher.register_command("SET", adapt(commands::set));
    dispatcher.register_command("GET", adapt(commands::get));

    dispatcher.register_command("LLEN", adapt(commands::llen));
    dispatcher.register_command("RPUSH", adapt(commands::rpush));
    dispatcher.register_command("LPUSH", adapt(commands::lpush));
    dispatcher.register_command("LRANGE", adapt(commands::lrange));
    dispatcher.register_command("LPOP", adapt(commands::lpop));
    dispatcher.register_command("BLPOP", adapt(commands::blpop));

    dispatcher.register_command("XADD", adapt(commands::xadd));
    dispatcher.register_command("XRANGE", adapt(commands::xrange));
    dispatcher.register_command("XREAD", adapt(commands::xread));
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

  RESPValue rpush(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager)
  {
    // RPUSH key element [element ...]
    if (value.array.size() < 3)
      return RESPValue::Error("wrong number of arguments for 'RPUSH' command");

    const std::string &key = value.array[1].str;
    std::vector<std::string> values;
    for (size_t i = 2; i < value.array.size(); ++i)
      values.push_back(value.array[i].str);

    RESPValue result = store.rpush(key, values);
    if (result.type != RESPValue::Type::Error)
    {
      blocking_manager.unblock_first_client_for_key(key);
    }

    return result;
  }

  RESPValue lpush(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager)
  {
    // LPUSH key element [element ...]
    if (value.array.size() < 3)
      return RESPValue::Error("wrong number of arguments for 'LPUSH' command");

    const std::string &key = value.array[1].str;
    std::vector<std::string> values;
    for (size_t i = 2; i < value.array.size(); ++i)
      values.push_back(value.array[i].str);

    RESPValue result = store.lpush(key, values);
    if (result.type != RESPValue::Type::Error)
    {
      blocking_manager.unblock_first_client_for_key(key);
    }

    return result;
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

  RESPValue blpop(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager, int client_fd)
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

    RESPValue result = store.blpop(key);

    if (result.type == RESP_BLOCK_CLIENT.type && result.str == RESP_BLOCK_CLIENT.str)
    {
      // Register this client as waiting
      blocking_manager.block_client(client_fd, {key}, timeout * 1000);
      RESPValue command = RESPValue::Array({RESPValue::BulkString("LPOP"), RESPValue::BulkString(key)});

      return RESPValue::Array({
          std::move(result), // Signal
          std::move(command) // Command to save
      });
    }

    return result;
  }

  RESPValue xadd(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager)
  {
    // XADD key id field1 value1 [field2 value2 ...]
    if (value.array.size() < 5 || (value.array.size() - 3) % 2 != 0)
      return RESPValue::Error("wrong number of arguments for 'XADD' command");

    StreamEntry entry;
    for (size_t i = 3; i < value.array.size(); i++)
      entry.push_back(value.array[i].str);

    const std::string &key = value.array[1].str;
    const std::string &id_str = value.array[2].str;
    RESPValue result = store.xadd(key, id_str, entry);

    if (result.type != RESPValue::Type::Error)
    {
      // Let main loop re-process unblocked clients
      blocking_manager.unblock_clients_for_key(key);
    }

    return result;
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

  RESPValue xread(const RESPValue &value, DataStore &store, BlockingManager &blocking_manager, int client_fd)
  {
    // Basic: XREAD STREAMS key id
    // Full: XREAD [COUNT count] [BLOCK ms] STREAMS key [key ...] id [id ...]
    int64_t block_ms = -1;
    size_t streams_pos = 0;
    for (size_t i = 1; i < value.array.size(); ++i)
    {
      std::string opt = to_upper(value.array[i].str);
      if (opt == "BLOCK")
      {
        if (i + 1 >= value.array.size())
          return RESPValue::Error("syntax error");
        try
        {
          block_ms = std::stoll(value.array[i + 1].str);
        }
        catch (...)
        {
          return RESPValue::Error("timeout is not an integer");
        }
        i++;
      }
      else if (opt == "STREAMS")
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
      const std::string &key = value.array[streams_pos + 1 + i].str;
      const std::string &id_str = value.array[streams_pos + 1 + num_streams + i].str;

      keys.push_back(key);
      if (id_str == "$")
      {
        const std::string &key_for_this_id = keys[i];
        auto last_id_opt = store.get_last_stream_id(key);
        if (last_id_opt)
          ids.push_back(last_id_opt->to_string());
        else
          ids.push_back("0-0");
      }
      else
        ids.push_back(id_str);
    }

    RESPValue result = store.xread(keys, ids, block_ms);

    if (result.type == RESP_BLOCK_CLIENT.type && result.str == RESP_BLOCK_CLIENT.str)
    {
      // Register this client as waiting
      blocking_manager.block_client(client_fd, keys, block_ms);

      std::vector<RESPValue> commands;
      commands.reserve(value.array.size());

      // XREAD [COUNT count] [BLOCK ms] STREAMS
      for (size_t i = 0; i <= streams_pos; ++i)
        commands.push_back(value.array[i]);

      // key [key ...] id [id ...]
      for (int i = 0; i < num_streams; ++i)
      {
        commands.push_back(RESPValue::BulkString(keys[i]));
        commands.push_back(RESPValue::BulkString(ids[i]));
      }

      RESPValue resolved_command = RESPValue::Array(std::move(commands));
      return RESPValue::Array({
          std::move(result),          // Signal
          std::move(resolved_command) // Command to save
      });
    }

    return result;
  }
}

std::string to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}