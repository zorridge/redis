#pragma once

#include "../command/command_dispatcher.hpp"
#include "../socket/socket.hpp"

void handle_client(SocketRAII client_fd, CommandDispatcher &dispatcher);
