#include "resp_serializer.hpp"

std::string RESPSerializer::serialize(const RESPValue &value)
{
  using Type = RESPValue::Type;

  switch (value.type)
  {
  case Type::SimpleString:
    return "+" + value.str + "\r\n";
  case Type::Error:
    return "-" + value.str + "\r\n";
  case Type::Integer:
    return ":" + std::to_string(value.integer) + "\r\n";
  case Type::BulkString:
    return "$" + std::to_string(value.str.size()) + "\r\n" + value.str + "\r\n";
  case Type::Null:
    return "$-1\r\n";
  case Type::Array:
  {
    if (value.array.empty())
      return "*0\r\n";

    std::string out = "*" + std::to_string(value.array.size()) + "\r\n";
    for (const auto &v : value.array)
    {
      out += serialize(v);
    }

    return out;
  }
  default:
    return "-unknown type\r\n";
  }
}
