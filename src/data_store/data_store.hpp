#pragma once

#include "../resp/resp_value.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class DataStore
{
public:
  using Value = std::variant<std::string, std::deque<std::string>>;

  RESPValue set(const std::string &key,
                const std::string &value,
                std::chrono::milliseconds ttl = std::chrono::milliseconds(0));
  RESPValue get(const std::string &key);

  RESPValue llen(const std::string &key);
  RESPValue rpush(const std::string &key, const std::vector<std::string> &values);
  RESPValue lpush(const std::string &key, const std::vector<std::string> &values);
  RESPValue lrange(const std::string &key, int64_t start, int64_t stop);
  RESPValue lpop(const std::string &key, int64_t count = 1);
  RESPValue blpop(const std::string &key, double timeout_seconds);

private:
  struct Entry
  {
    Value value;
    std::chrono::steady_clock::time_point expire_at; // time_point::max() if no expiry
  };
  std::unordered_map<std::string, Entry> m_store;
  std::mutex m_mutex;

  bool is_expired(const Entry &entry) const;

  std::unordered_map<std::string, std::shared_ptr<std::condition_variable>> m_key_cvs;
  std::shared_ptr<std::condition_variable> get_cv_for_key(const std::string &key);
  void notify_list_push(const std::string &key);
};
