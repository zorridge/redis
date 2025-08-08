#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

struct RESPValue
{
  enum class Type
  {
    SimpleString,
    Error,
    Integer,
    BulkString,
    Array,
    Null
  };

  Type type = Type::Null;
  std::string str;
  int64_t integer = 0;
  std::vector<RESPValue> array;

  RESPValue() = default;
  ~RESPValue() = default;

  RESPValue(const RESPValue &other) = default;
  RESPValue &operator=(const RESPValue &other) = default;

  // Enable move
  RESPValue(RESPValue &&other) noexcept
      : type(other.type),
        str(std::move(other.str)),
        integer(other.integer),
        array(std::move(other.array)) {}
  RESPValue &operator=(RESPValue &&other) noexcept
  {
    if (this != &other)
    {
      type = other.type;
      str = std::move(other.str);
      integer = other.integer;
      array = std::move(other.array);
    }
    return *this;
  }

  static RESPValue SimpleString(const std::string &s)
  {
    RESPValue v;
    v.type = Type::SimpleString;
    v.str = s;
    return v;
  }
  static RESPValue SimpleString(std::string &&s)
  { // Move overload
    RESPValue v;
    v.type = Type::SimpleString;
    v.str = std::move(s);
    return v;
  }

  static RESPValue Error(const std::string &s)
  {
    RESPValue v;
    v.type = Type::Error;
    v.str = s;
    return v;
  }
  static RESPValue Error(std::string &&s)
  { // Move overload
    RESPValue v;
    v.type = Type::Error;
    v.str = std::move(s);
    return v;
  }

  static RESPValue Integer(int64_t i)
  {
    RESPValue v;
    v.type = Type::Integer;
    v.integer = i;
    return v;
  }

  static RESPValue BulkString(const std::string &s)
  {
    RESPValue v;
    v.type = Type::BulkString;
    v.str = s;
    return v;
  }
  static RESPValue BulkString(std::string &&s)
  { // Move overload
    RESPValue v;
    v.type = Type::BulkString;
    v.str = std::move(s);
    return v;
  }

  static RESPValue Null()
  {
    RESPValue v;
    v.type = Type::Null;
    return v;
  }

  static RESPValue Array(const std::vector<RESPValue> &arr)
  {
    RESPValue v;
    v.type = Type::Array;
    v.array = arr;
    return v;
  }
  static RESPValue Array(std::vector<RESPValue> &&arr)
  { // Move overload
    RESPValue v;
    v.type = Type::Array;
    v.array = std::move(arr);
    return v;
  }
};
