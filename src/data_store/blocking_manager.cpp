#include "data_store.hpp"

const RESPValue RESP_BLOCK_CLIENT = RESPValue::SimpleString("__BLOCK__");

void DataStore::BlockingManager::block_client(int client_fd, const std::vector<std::string> &keys, int64_t timeout_ms)
{
  BlockedClient client;
  client.fd = client_fd;
  client.keys = keys;
  client.has_timeout = (timeout_ms > 0);

  if (client.has_timeout && timeout_ms > 0)
  {
    client.timeout_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  }

  // Add to the main details map for easy lookup
  m_waiter_details[client_fd] = client;

  // Add the client to the waiting queue for each key
  for (const auto &key : keys)
    m_key_to_waiters[key].push(client_fd);
}

std::vector<int> DataStore::BlockingManager::unblock_clients_for_key(const std::string &key)
{
  auto it = m_key_to_waiters.find(key);
  if (it == m_key_to_waiters.end())
  {
    return {}; // No one is waiting on this key
  }

  std::vector<int> unblocked_fds;
  std::queue<int> &waiters = it->second;
  while (!waiters.empty())
  {
    int client_fd = waiters.front();
    waiters.pop();

    if (m_waiter_details.erase(client_fd) > 0)
      unblocked_fds.push_back(client_fd);
  }
  m_key_to_waiters.erase(it);

  return unblocked_fds;
}

std::vector<int> DataStore::BlockingManager::find_and_clear_timed_out_clients()
{
  std::vector<int> timed_out_fds;
  if (m_waiter_details.empty())
    return timed_out_fds;

  auto now = std::chrono::steady_clock::now();
  for (auto it = m_waiter_details.begin(); it != m_waiter_details.end();)
  {
    const auto &client = it->second;
    if (client.has_timeout && client.timeout_at <= now)
    {
      timed_out_fds.push_back(client.fd);
      it = m_waiter_details.erase(it);
    }
    else
      ++it;
  }

  return timed_out_fds;
}
