#pragma once

#include "../resp/resp_value.hpp"

#include <chrono>
#include <list>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

extern const RESPValue RESP_BLOCK_CLIENT;

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

  void block_client(int client_fd, const std::vector<std::string> &keys, int64_t block_ms);
  void unblock_clients_for_key(const std::string &key);
  void unblock_first_client_for_key(const std::string &key);
  std::vector<int> find_and_clear_timed_out_clients();

  std::list<int> &get_ready_list() { return m_ready_list; }
  const std::list<int> &get_ready_list() const { return m_ready_list; }
  void clear_ready_list() { m_ready_list.clear(); }

private:
  std::list<int> m_ready_list;

  // key -> queue of client_fd waiting
  std::unordered_map<std::string, std::queue<int>> m_key_to_waiters;
  // client_fd -> blocking details
  std::unordered_map<int, BlockedClient> m_waiter_details;
};