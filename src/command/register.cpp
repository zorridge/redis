#include "register.hpp"
#include "commands.hpp"

void register_all_commands(CommandDispatcher &dispatcher, DataStore &store)
{
  dispatcher.register_command("PING", commands::ping);
  dispatcher.register_command("COMMAND", commands::command);
  dispatcher.register_command("ECHO", commands::echo);

  dispatcher.register_command("SET", [&store](const RESPValue &value) -> RESPValue
                              { return commands::set(value, store); });
  dispatcher.register_command("GET", [&store](const RESPValue &value) -> RESPValue
                              { return commands::get(value, store); });
}
