#pragma once

#include "types.hpp"
#include "../event_loop/blocking_manager.hpp"
#include "../resp/resp_value.hpp"

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

class DataStore
{
public:
  RESPValue type(const std::string &key);

  RESPValue set(const std::string &key, const std::string &value, std::chrono::milliseconds ttl = std::chrono::milliseconds(0));
  RESPValue get(const std::string &key);

  RESPValue llen(const std::string &key);
  RESPValue rpush(const std::string &key, const std::vector<std::string> &values);
  RESPValue lpush(const std::string &key, const std::vector<std::string> &values);
  RESPValue lrange(const std::string &key, int64_t start, int64_t stop);
  RESPValue lpop(const std::string &key, int64_t count = 1);
  RESPValue blpop(const std::string &key);

  RESPValue xadd(const std::string &key, const std::string &id, const StreamEntry &entry);
  RESPValue xrange(const std::string &key, const std::string &start_id_str, const std::string &end_id_str, int64_t count);
  RESPValue xread(const std::vector<std::string> &keys, const std::vector<std::string> &ids, int64_t block_ms);

  std::optional<StreamID> get_last_stream_id(const std::string &key) const;

private:
  std::unordered_map<std::string, Entry> m_store;
  bool is_expired(const Entry &entry) const;
};
