#pragma once

#include "types.hpp"
#include "../resp/resp_value.hpp"

#include <chrono>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

extern const RESPValue RESP_BLOCK_CLIENT;

class DataStore
{
private:
  // Manages client blocking logic
  class BlockingManager
  {
  public:
    struct BlockedClient
    {
      int fd;
      std::vector<std::string> keys;
      std::chrono::steady_clock::time_point timeout_at;
      bool has_timeout;
    };

    void block_client(int client_fd, const std::vector<std::string> &keys, int64_t timeout_ms);
    std::vector<int> unblock_clients_for_key(const std::string &key);
    std::vector<int> find_and_clear_timed_out_clients();

  private:
    // key -> queue of client_fd waiting
    std::unordered_map<std::string, std::queue<int>> m_key_to_waiters;
    // client_fd -> blocking details
    std::unordered_map<int, BlockedClient> m_waiter_details;
  };

public:
  RESPValue type(const std::string &key);

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

  RESPValue xadd(const std::string &key, const std::string &id, const StreamEntry &entry);
  RESPValue xrange(const std::string &key,
                   const std::string &start_id_str,
                   const std::string &end_id_str,
                   int64_t count);
  RESPValue xread(const std::vector<std::string> &keys,
                  const std::vector<std::string> &ids,
                  int64_t block_ms,
                  int client_fd);

  BlockingManager &get_blocking_manager() { return m_blocking_manager; }
  std::optional<StreamID> get_last_stream_id(const std::string &key) const;

private:
  std::unordered_map<std::string, Entry> m_store;
  std::mutex m_mutex;

  BlockingManager m_blocking_manager;

  bool is_expired(const Entry &entry) const;

  std::unordered_map<std::string, std::shared_ptr<std::condition_variable>> m_key_cvs;
  std::shared_ptr<std::condition_variable> get_cv_for_key(const std::string &key);
  void notify_list_push(const std::string &key);
};
