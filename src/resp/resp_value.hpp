#pragma once

#include <string>
#include <vector>
#include <cstdint>

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

  Type type;
  std::string str;
  int64_t integer = 0;
  std::vector<RESPValue> array;

  static RESPValue SimpleString(const std::string &s)
  {
    RESPValue v;
    v.type = Type::SimpleString;
    v.str = s;
    return v;
  }
  static RESPValue Error(const std::string &s)
  {
    RESPValue v;
    v.type = Type::Error;
    v.str = s;
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
};
