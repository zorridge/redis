#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>

#include "../resp/resp_value.hpp"

class DataStore
{
public:
  RESPValue set(const std::string &key,
                const std::string &value,
                std::chrono::milliseconds ttl = std::chrono::milliseconds(0));

  RESPValue get(const std::string &key);

private:
  struct Entry
  {
    std::string value;
    std::chrono::steady_clock::time_point expire_at; // time_point::max() if no expiry
  };

  std::unordered_map<std::string, Entry> m_store;
  std::mutex m_mutex;
};
