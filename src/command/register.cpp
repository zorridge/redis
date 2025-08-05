#include "register.hpp"
#include "commands.hpp"

void register_all_commands(CommandDispatcher &dispatcher)
{
  dispatcher.register_command("PING", commands::ping);
  dispatcher.register_command("COMMAND", commands::command);
  dispatcher.register_command("ECHO", commands::echo);

  dispatcher.register_command("SET", [&dispatcher](const RESPValue &value) -> RESPValue
                              { return commands::set(value, dispatcher.get_store()); });
  dispatcher.register_command("GET", [&dispatcher](const RESPValue &value) -> RESPValue
                              { return commands::get(value, dispatcher.get_store()); });
}
