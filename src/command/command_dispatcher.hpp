#pragma once

#include <string>
#include <unordered_map>
#include <functional>

#include "../resp/resp_value.hpp"
#include "../data_store/data_store.hpp"

class CommandDispatcher
{
public:
  using Handler = std::function<RESPValue(const RESPValue &)>;

  CommandDispatcher(DataStore &store);

  void register_command(const std::string &cmd, Handler handler);
  std::string dispatch(const RESPValue &value) const;

  DataStore &get_store() { return m_store; }

private:
  std::unordered_map<std::string, Handler> m_handlers;
  DataStore &m_store;

  static std::string to_upper(const std::string &s);
};
