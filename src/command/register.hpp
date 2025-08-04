#pragma once
#include "command_dispatcher.hpp"
#include "../data_store/data_store.hpp"

void register_all_commands(CommandDispatcher &dispatcher, DataStore &store);
