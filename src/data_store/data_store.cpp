#include "data_store.hpp"

RESPValue DataStore::set(const std::string &key,
                         const std::string &value,
                         std::chrono::milliseconds ttl)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  Entry entry;
  entry.value = value;
  if (ttl.count() > 0)
  {
    entry.expire_at = std::chrono::steady_clock::now() + ttl;
  }
  else
  {
    entry.expire_at = std::chrono::steady_clock::time_point::max();
  }

  m_store[key] = entry;
  return RESPValue::SimpleString("OK");
}

RESPValue DataStore::get(const std::string &key)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end())
    return RESPValue::Null();

  auto now = std::chrono::steady_clock::now();
  if (it->second.expire_at != std::chrono::steady_clock::time_point::max() &&
      now > it->second.expire_at)
  {
    m_store.erase(it);
    return RESPValue::Null();
  }

  return RESPValue::BulkString(it->second.value);
}
