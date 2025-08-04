#pragma once
#include <string>
#include "resp_value.hpp"

class RESPParser
{
public:
  void feed(const char *data, size_t len);
  RESPValue parse();

private:
  std::string m_buffer;

  RESPValue parse_value(size_t &pos);
  RESPValue parse_simple_string(size_t &pos);
  RESPValue parse_error(size_t &pos);
  RESPValue parse_integer(size_t &pos);
  RESPValue parse_bulk_string(size_t &pos);
  RESPValue parse_array(size_t &pos);
};
