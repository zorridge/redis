#pragma once

#include "../event_loop/blocking_manager.hpp"
#include "../data_store/data_store.hpp"
#include "../resp/resp_value.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <list>

class ClientHandler;

class CommandDispatcher
{
public:
  using Handler = std::function<RESPValue(const RESPValue &, ClientHandler &)>;

  CommandDispatcher(DataStore &store, BlockingManager &blocking_manager);

  void register_command(const std::string &cmd, Handler handler);
  RESPValue dispatch(const RESPValue &value, ClientHandler &client) const;

  DataStore &get_store() { return m_store; }
  BlockingManager &get_blocking_manager() { return m_blocking_manager; }

private:
  std::unordered_map<std::string, Handler> m_handlers;
  DataStore &m_store;
  BlockingManager &m_blocking_manager;

  static std::string to_upper(const std::string &s);
};
