#pragma once
#include <string>
#include "resp_value.hpp"

class RESPSerializer
{
public:
  static std::string serialize(const RESPValue &value);
};
