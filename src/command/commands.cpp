#include "commands.hpp"

std::string to_upper(const std::string &s);

namespace commands
{
  void register_all_commands(CommandDispatcher &dispatcher)
  {
    auto adapt = [&dispatcher](auto func)
    {
      return [&dispatcher, func](const RESPValue &value, ClientHandler &client) -> RESPValue
      {
        return func(value, client, dispatcher);
      };
    };

    dispatcher.register_command("PING", adapt(commands::ping));
    dispatcher.register_command("COMMAND", adapt(commands::command));
    dispatcher.register_command("ECHO", adapt(commands::echo));

    dispatcher.register_command("TYPE", adapt(commands::type));

    dispatcher.register_command("SET", adapt(commands::set));
    dispatcher.register_command("GET", adapt(commands::get));
    dispatcher.register_command("INCR", adapt(commands::incr));

    dispatcher.register_command("LLEN", adapt(commands::llen));
    dispatcher.register_command("RPUSH", adapt(commands::rpush));
    dispatcher.register_command("LPUSH", adapt(commands::lpush));
    dispatcher.register_command("LRANGE", adapt(commands::lrange));
    dispatcher.register_command("LPOP", adapt(commands::lpop));
    dispatcher.register_command("BLPOP", adapt(commands::blpop));

    dispatcher.register_command("XADD", adapt(commands::xadd));
    dispatcher.register_command("XRANGE", adapt(commands::xrange));
    dispatcher.register_command("XREAD", adapt(commands::xread));

    dispatcher.register_command("MULTI", adapt(commands::multi));
    dispatcher.register_command("EXEC", adapt(commands::exec));
    dispatcher.register_command("DISCARD", adapt(commands::discard));

    dispatcher.register_command("CONFIG", adapt(commands::config));

    dispatcher.register_command("SUBSCRIBE", adapt(commands::subscribe));
    dispatcher.register_command("UNSUBSCRIBE", adapt(commands::unsubscribe));
    dispatcher.register_command("PUBLISH", adapt(commands::publish));
  }

  RESPValue ping(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (value.array.size() == 1)
    {
      if (client.is_subscriber_mode())
      {
        return RESPValue::Array({RESPValue::BulkString("pong"), RESPValue::BulkString("")});
      }
      else
        return RESPValue::SimpleString("PONG");
    }
    if (value.array.size() == 2)
      return value.array[1];
    return RESPValue::Error("ERR wrong number of arguments for 'ping' command");
  }

  RESPValue command(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    return RESPValue::Array({});
  }

  RESPValue echo(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (value.array.size() != 2)
      return RESPValue::Error("ERR wrong number of arguments for 'echo' command");
    return value.array[1];
  }

  RESPValue type(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // TYPE key
    if (value.array.size() != 2)
      return RESPValue::Error("ERR wrong number of arguments for 'type' command");

    DataStore &store = dispatcher.get_store();
    return store.type(value.array[1].str);
  }

  RESPValue set(const RESPValue &value, ClientHandler &, CommandDispatcher &dispatcher)
  {
    // SET key value [PX milliseconds]
    if (value.array.size() != 3 && value.array.size() != 5)
      return RESPValue::Error("ERR wrong number of arguments for 'set' command");

    DataStore &store = dispatcher.get_store();
    if (value.array.size() == 3)
    {
      return store.set(value.array[1].str, value.array[2].str);
    }

    if (to_upper(value.array[3].str) != "PX")
      return RESPValue::Error("ERR syntax error");

    try
    {
      std::chrono::milliseconds ttl{std::stoll(value.array[4].str)};
      if (ttl.count() <= 0)
        return RESPValue::Error("ERR invalid expire time in 'set' command");
      return store.set(value.array[1].str, value.array[2].str, ttl);
    }
    catch (...)
    {
      return RESPValue::Error("ERR value is not an integer or out of range");
    }
  }

  RESPValue get(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (value.array.size() != 2)
      return RESPValue::Error("ERR wrong number of arguments for 'get' command");

    DataStore &store = dispatcher.get_store();
    return store.get(value.array[1].str);
  }

  RESPValue incr(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (value.array.size() != 2)
      return RESPValue::Error("ERR wrong number of arguments for 'incr' command");

    DataStore &store = dispatcher.get_store();
    return store.incr(value.array[1].str);
  }

  RESPValue llen(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // LLEN key
    if (value.array.size() != 2)
      return RESPValue::Error("ERR wrong number of arguments for 'llen' command");

    DataStore &store = dispatcher.get_store();
    const std::string &key = value.array[1].str;
    return store.llen(key);
  }

  RESPValue rpush(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // RPUSH key element [element ...]
    if (value.array.size() < 3)
      return RESPValue::Error("ERR wrong number of arguments for 'rpush' command");

    const std::string &key = value.array[1].str;
    std::vector<std::string> values;
    for (size_t i = 2; i < value.array.size(); ++i)
      values.push_back(value.array[i].str);

    DataStore &store = dispatcher.get_store();
    RESPValue result = store.rpush(key, values);
    if (result.type != RESPValue::Type::Error)
    {
      BlockingManager &blocking_manager = dispatcher.get_blocking_manager();
      blocking_manager.unblock_first_client_for_key(key);
    }

    return result;
  }

  RESPValue lpush(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // LPUSH key element [element ...]
    if (value.array.size() < 3)
      return RESPValue::Error("ERR wrong number of arguments for 'lpush' command");

    const std::string &key = value.array[1].str;
    std::vector<std::string> values;
    for (size_t i = 2; i < value.array.size(); ++i)
      values.push_back(value.array[i].str);

    DataStore &store = dispatcher.get_store();
    RESPValue result = store.lpush(key, values);
    if (result.type != RESPValue::Type::Error)
    {
      BlockingManager &blocking_manager = dispatcher.get_blocking_manager();
      blocking_manager.unblock_first_client_for_key(key);
    }

    return result;
  }

  RESPValue lrange(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // LRANGE key start stop
    if (value.array.size() != 4)
      return RESPValue::Error("ERR wrong number of arguments for 'lrange' command");

    const std::string &key = value.array[1].str;
    try
    {
      int64_t start = std::stoll(value.array[2].str);
      int64_t stop = std::stoll(value.array[3].str);

      DataStore &store = dispatcher.get_store();
      return store.lrange(key, start, stop);
    }
    catch (...)
    {
      return RESPValue::Error("ERR value is not an integer or out of range");
    }
  }

  RESPValue lpop(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // LPOP key [count]
    if (value.array.size() < 2 || value.array.size() > 3)
      return RESPValue::Error("ERR wrong number of arguments for 'lpop' command");

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
        return RESPValue::Error("ERR value is not an integer or out of range");
      }

      if (count < 0)
        return RESPValue::Error("ERR value is out of range, must be positive");
      if (count == 0)
        return RESPValue::Array({});
    }

    DataStore &store = dispatcher.get_store();
    return store.lpop(key, count);
  }

  RESPValue blpop(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // BLPOP key timeout
    if (value.array.size() != 3)
      return RESPValue::Error("ERR wrong number of arguments for 'blpop' command");

    const std::string &key = value.array[1].str;
    double timeout = 0.0;
    try
    {
      timeout = std::stod(value.array[2].str);
      if (timeout < 0.0)
        return RESPValue::Error("ERR timeout is negative");
    }
    catch (...)
    {
      return RESPValue::Error("ERR timeout is not a float or out of range");
    }

    DataStore &store = dispatcher.get_store();
    RESPValue result = store.blpop(key);

    if (result.type == RESP_BLOCK_CLIENT.type && result.str == RESP_BLOCK_CLIENT.str)
    {
      // Register this client as waiting
      BlockingManager &blocking_manager = dispatcher.get_blocking_manager();
      blocking_manager.block_client(client.get_fd(), {key}, timeout * 1000);
      RESPValue command = RESPValue::Array({RESPValue::BulkString("LPOP"), RESPValue::BulkString(key)});

      return RESPValue::Array({
          std::move(result), // Signal
          std::move(command) // Command to save
      });
    }

    return result;
  }

  RESPValue xadd(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // XADD key id field1 value1 [field2 value2 ...]
    if (value.array.size() < 5 || (value.array.size() - 3) % 2 != 0)
      return RESPValue::Error("ERR wrong number of arguments for 'xadd' command");

    StreamEntry entry;
    for (size_t i = 3; i < value.array.size(); i++)
      entry.push_back(value.array[i].str);

    const std::string &key = value.array[1].str;
    const std::string &id_str = value.array[2].str;

    DataStore &store = dispatcher.get_store();
    RESPValue result = store.xadd(key, id_str, entry);

    if (result.type != RESPValue::Type::Error)
    {
      // Let main loop re-process unblocked clients
      BlockingManager &blocking_manager = dispatcher.get_blocking_manager();
      blocking_manager.unblock_clients_for_key(key);
    }

    return result;
  }

  RESPValue xrange(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // XRANGE key start end [COUNT count]
    if (value.array.size() < 4)
      return RESPValue::Error("ERR wrong number of arguments for 'xrange' command");

    int64_t count = -1;
    if (value.array.size() > 4)
    {
      if (value.array.size() != 6 || value.array[4].str != "COUNT")
        return RESPValue::Error("ERR syntax error");

      try
      {
        count = std::stoll(value.array[5].str);
      }
      catch (...)
      {
        return RESPValue::Error("ERR value is not an integer or out of range");
      }
    }

    DataStore &store = dispatcher.get_store();
    return store.xrange(value.array[1].str, value.array[2].str, value.array[3].str, count);
  }

  RESPValue xread(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
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
          return RESPValue::Error("ERR syntax error");
        try
        {
          block_ms = std::stoll(value.array[i + 1].str);
          if (block_ms < 0)
            return RESPValue::Error("ERR timeout is negative");
        }
        catch (...)
        {
          return RESPValue::Error("ERR value is not an integer or out of range");
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
      return RESPValue::Error("ERR Unrecognized option or wrong number of arguments for 'xread' command");

    size_t args_after_streams = value.array.size() - (streams_pos + 1);
    if (args_after_streams == 0 || args_after_streams % 2 != 0)
      return RESPValue::Error("ERR Unbalanced XREAD list of streams: keys and IDs must be provided in pairs.");

    size_t num_streams = args_after_streams / 2;
    std::vector<std::string> keys;
    std::vector<std::string> ids;
    keys.reserve(num_streams);
    ids.reserve(num_streams);

    DataStore &store = dispatcher.get_store();

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
      BlockingManager &blocking_manager = dispatcher.get_blocking_manager();
      blocking_manager.block_client(client.get_fd(), keys, block_ms);

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

  RESPValue multi(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (client.is_in_transaction())
      return RESPValue::Error("ERR MULTI calls can not be nested");

    client.start_transaction();
    return RESPValue::SimpleString("OK");
  }

  RESPValue discard(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (!client.is_in_transaction())
      return RESPValue::Error("ERR DISCARD without MULTI");

    client.clear_transaction_state();
    return RESPValue::SimpleString("OK");
  }

  RESPValue exec(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    if (!client.is_in_transaction())
      return RESPValue::Error("ERR EXEC without MULTI");

    const auto &command_queue = client.get_command_queue();
    std::vector<RESPValue> results;
    results.reserve(command_queue.size());

    for (const auto &command : command_queue)
    {
      results.push_back(dispatcher.dispatch(command, client));
    }

    client.clear_transaction_state();
    return RESPValue::Array(std::move(results));
  }

  RESPValue config(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // CONFIG GET parameter
    if (value.array.size() != 3)
      return RESPValue::Error("ERR wrong number of arguments for 'config' command");

    std::string sub = to_upper(value.array[1].str);
    if (sub != "GET")
      return RESPValue::Error("ERR unknown subcommand" + sub);

    std::string param = value.array[2].str;
    const Config &config = dispatcher.get_config();
    if (param == "dir")
    {
      return RESPValue::Array({RESPValue::BulkString(param), RESPValue::BulkString(config.dir)});
    }
    else if (param == "dbfilename")
    {
      return RESPValue::Array({RESPValue::BulkString(param), RESPValue::BulkString(config.dbfilename)});
    }

    return RESPValue::Array({});
  }

  RESPValue subscribe(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // SUBSCRIBE channel [channel ...]
    if (value.array.size() < 2)
      return RESPValue::Error("ERR wrong number of arguments for 'subscribe' command");

    PubSubManager &pubsub = dispatcher.get_pubsub_manager();
    int fd = client.get_fd();
    std::vector<RESPValue> confirmations;

    for (size_t i = 1; i < value.array.size(); ++i)
    {
      const std::string &channel = value.array[i].str;
      client.add_subscription(channel);
      pubsub.subscribe(&client, channel);

      size_t count = client.get_subscribed_channels().size();
      confirmations.push_back(RESPValue::Array({RESPValue::BulkString("subscribe"),
                                                RESPValue::BulkString(channel),
                                                RESPValue::Integer(static_cast<int64_t>(count))}));
    }

    return confirmations.size() == 1 ? confirmations[0] : RESPValue::Array(std::move(confirmations));
  }

  RESPValue unsubscribe(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // UNSUBSCRIBE channel [channel ...]
    PubSubManager &pubsub = dispatcher.get_pubsub_manager();
    int fd = client.get_fd();
    std::vector<RESPValue> confirmations;

    // If no channels specified, unsubscribe from all
    if (value.array.size() == 1)
    {
      const auto &channels = client.get_subscribed_channels();
      if (channels.empty())
      {
        confirmations.push_back(RESPValue::Array({RESPValue::BulkString("unsubscribe"),
                                                  RESPValue::BulkString(""),
                                                  RESPValue::Integer(0)}));

        return RESPValue::Array(std::move(confirmations));
      }

      std::vector<std::string> channels_copy(channels.begin(), channels.end());
      for (const std::string &channel : channels_copy)
      {
        client.remove_subscription(channel);
        pubsub.unsubscribe(&client, channel);
        size_t count = client.get_subscribed_channels().size();
        confirmations.push_back(RESPValue::Array({RESPValue::BulkString("unsubscribe"),
                                                  RESPValue::BulkString(channel),
                                                  RESPValue::Integer(static_cast<int64_t>(count))}));
      }

      return RESPValue::Array(std::move(confirmations));
    }

    // Unsubscribe from specified channels
    for (size_t i = 1; i < value.array.size(); ++i)
    {
      const std::string &channel = value.array[i].str;
      client.remove_subscription(channel);
      pubsub.unsubscribe(&client, channel);
      size_t count = client.get_subscribed_channels().size();
      confirmations.push_back(RESPValue::Array({RESPValue::BulkString("unsubscribe"),
                                                RESPValue::BulkString(channel),
                                                RESPValue::Integer(static_cast<int64_t>(count))}));
    }

    return RESPValue::Array(std::move(confirmations));
  }

  RESPValue publish(const RESPValue &value, ClientHandler &client, CommandDispatcher &dispatcher)
  {
    // PUBLISH channel message
    if (value.array.size() != 3)
      return RESPValue::Error("ERR wrong number of arguments for 'publish' command");

    PubSubManager &pubsub = dispatcher.get_pubsub_manager();
    const std::string &channel = value.array[1].str;
    const std::string &message = value.array[2].str;
    int receivers = pubsub.publish(channel, message);

    return RESPValue::Integer(receivers);
  }
}

std::string to_upper(const std::string &s)
{
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}