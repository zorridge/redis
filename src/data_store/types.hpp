#pragma once

#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct StreamID
{
  uint64_t ms;
  uint64_t seq;
  bool operator<(const StreamID &other) const
  {
    return (ms < other.ms) || (ms == other.ms && seq < other.seq);
  }
  bool operator>(const StreamID &other) const
  {
    return (ms > other.ms) || (ms == other.ms && seq > other.seq);
  }
  bool operator==(const StreamID &other) const
  {
    return ms == other.ms && seq == other.seq;
  }
  bool operator<=(const StreamID &other) const
  {
    return *this < other || *this == other;
  }
  bool operator>=(const StreamID &other) const
  {
    return *this > other || *this == other;
  }
  std::string to_string() const
  {
    return std::to_string(ms) + "-" + std::to_string(seq);
  }
};
using StreamEntry = std::vector<std::string>;
using RedisStream = std::map<StreamID, StreamEntry>;

using RedisString = std::string;
using RedisList = std::deque<std::string>;

using Value = std::variant<RedisString, RedisList, RedisStream>;

struct Entry
{
  Value value;
  std::chrono::steady_clock::time_point expire_at; // time_point::max() if no expiry

  // Constructors
  Entry() = default;
  Entry(Value &&v, std::chrono::steady_clock::time_point exp)
      : value(std::move(v)), expire_at(exp)
  {
  }

  // Disable copy
  Entry(const Entry &) = delete;
  Entry &operator=(const Entry &) = delete;

  // Enable move
  Entry(Entry &&other) noexcept
      : value(std::move(other.value)),
        expire_at(std::move(other.expire_at))
  {
  }
  Entry &operator=(Entry &&other) noexcept
  {
    if (this != &other)
    {
      value = std::move(other.value);
      expire_at = std::move(other.expire_at);
    }
    return *this;
  }
};