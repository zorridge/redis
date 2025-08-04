#include <cstdlib>
#include <optional>

#include "resp_parser.hpp"
#include "../utils/line_reader.hpp"

void RESPParser::feed(const char *data, size_t len)
{
  m_buffer.assign(data, len);
}

RESPValue RESPParser::parse()
{
  size_t pos = 0;
  RESPValue val = parse_value(pos);
  m_buffer.clear();
  return val;
}

RESPValue RESPParser::parse_value(size_t &pos)
{
  if (pos >= m_buffer.size())
    return RESPValue::Error("incomplete data");

  char type = m_buffer[pos];
  ++pos;

  switch (type)
  {
  case '+':
    return parse_simple_string(pos);
  case '-':
    return parse_error(pos);
  case ':':
    return parse_integer(pos);
  case '$':
    return parse_bulk_string(pos);
  case '*':
    return parse_array(pos);
  default:
    return RESPValue::Error("protocol error: unknown type");
  }
}

// +OK\r\n
RESPValue RESPParser::parse_simple_string(size_t &pos)
{
  auto line = read_line(m_buffer, pos);
  if (!line)
    return RESPValue::Error("incomplete data");

  return RESPValue::SimpleString(*line);
}

// -Error message\r\n
RESPValue RESPParser::parse_error(size_t &pos)
{
  auto line = read_line(m_buffer, pos);
  if (!line)
    return RESPValue::Error("incomplete data");

  return RESPValue::Error(*line);
}

// :<value>\r\n
RESPValue RESPParser::parse_integer(size_t &pos)
{
  auto line = read_line(m_buffer, pos);
  if (!line)
    return RESPValue::Error("incomplete data");

  try
  {
    int64_t val = std::stoll(*line);
    return RESPValue::Integer(val);
  }
  catch (...)
  {
    return RESPValue::Error("protocol error: invalid integer");
  }
}

// $<length>\r\n<data>\r\n
RESPValue RESPParser::parse_bulk_string(size_t &pos)
{
  auto line = read_line(m_buffer, pos);
  if (!line)
    return RESPValue::Error("incomplete data");

  int64_t len = 0;
  try
  {
    len = std::stoll(*line);
  }
  catch (...)
  {
    return RESPValue::Error("protocol error: invalid bulk string length");
  }

  if (len == -1)
    return RESPValue::Null();

  if (m_buffer.size() < pos + len + 2)
    return RESPValue::Error("incomplete data");

  std::string val = m_buffer.substr(pos, len);
  pos += len;
  if (m_buffer.substr(pos, 2) != "\r\n")
    return RESPValue::Error("protocol error: bulk string missing CRLF");

  pos += 2;
  return RESPValue::BulkString(val);
}

// *<number-of-elements>\r\n<element-1>...<element-n>
RESPValue RESPParser::parse_array(size_t &pos)
{
  auto line = read_line(m_buffer, pos);
  if (!line)
    return RESPValue::Error("incomplete data");

  int64_t count = 0;
  try
  {
    count = std::stoll(*line);
  }
  catch (...)
  {
    return RESPValue::Error("protocol error: invalid array length");
  }

  if (count == -1)
    return RESPValue::Null();

  std::vector<RESPValue> arr;
  for (int64_t i = 0; i < count; ++i)
  {
    RESPValue v = parse_value(pos);
    if (v.type == RESPValue::Type::Error)
      return v; // propagate error (including incomplete data)

    arr.push_back(v);
  }

  return RESPValue::Array(arr);
}
