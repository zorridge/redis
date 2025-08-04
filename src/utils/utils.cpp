#include <iostream>

#include "utils.hpp"
#include "../resp/resp_value.hpp"

void printCommand(const RESPValue &value)
{
  if (value.type == RESPValue::Type::Array)
  {
    std::cout << "[Server] Received command: ";
    for (const auto &v : value.array)
    {
      if (v.type == RESPValue::Type::BulkString)
        std::cout << '"' << v.str << "\" ";
    }
    std::cout << std::endl;
  }
}