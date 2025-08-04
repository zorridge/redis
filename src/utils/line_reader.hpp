#pragma once
#include <string>
#include <optional>

inline std::optional<std::string> read_line(const std::string &buffer, size_t &pos)
{
  auto end = buffer.find("\r\n", pos);
  if (end == std::string::npos)
    return std::nullopt;

  std::string line = buffer.substr(pos, end - pos);
  pos = end + 2;
  return line;
}
