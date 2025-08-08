#pragma once

#include "../resp/resp_value.hpp"
#include "../data_store/data_store.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <list>

class CommandDispatcher
{
public:
  using Handler = std::function<RESPValue(const RESPValue &, int, std::list<int> &)>;

  CommandDispatcher(DataStore &store);

  void register_command(const std::string &cmd, Handler handler);
  RESPValue dispatch(const RESPValue &value, int client_fd, std::list<int> &ready_list) const;

  DataStore &get_store() { return m_store; }

private:
  std::unordered_map<std::string, Handler> m_handlers;
  DataStore &m_store;

  static std::string to_upper(const std::string &s);
};
