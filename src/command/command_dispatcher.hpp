#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include "../resp/resp_value.hpp"

class CommandDispatcher
{
public:
  using Handler = std::function<RESPValue(const RESPValue &)>;

  CommandDispatcher();
  void register_command(const std::string &cmd, Handler handler);
  std::string dispatch(const RESPValue &value) const;

private:
  std::unordered_map<std::string, Handler> m_handlers;

  static std::string to_upper(const std::string &s);
};
