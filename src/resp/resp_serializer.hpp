#pragma once

#include "resp_value.hpp"

#include <string>

class RESPSerializer
{
public:
  static std::string serialize(const RESPValue &value);
};
