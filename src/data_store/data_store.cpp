#include "data_store.hpp"

bool DataStore::is_expired(const Entry &entry) const
{
  auto now = std::chrono::steady_clock::now();
  return entry.expire_at != std::chrono::steady_clock::time_point::max() && now > entry.expire_at;
}

RESPValue DataStore::set(const std::string &key,
                         const std::string &value,
                         std::chrono::milliseconds ttl)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  Entry entry;
  entry.value = value;
  if (ttl.count() > 0)
    entry.expire_at = std::chrono::steady_clock::now() + ttl;
  else
    entry.expire_at = std::chrono::steady_clock::time_point::max();

  m_store[key] = entry;
  return RESPValue::SimpleString("OK");
}

RESPValue DataStore::get(const std::string &key)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end())
    return RESPValue::Null();

  if (is_expired(it->second))
  {
    m_store.erase(it);
    return RESPValue::Null();
  }

  if (auto *str = std::get_if<std::string>(&it->second.value))
  {
    return RESPValue::BulkString(*str);
  }
  else if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
  {
    std::vector<RESPValue> arr;
    for (const auto &s : *deq)
      arr.push_back(RESPValue::BulkString(s));
    return RESPValue::Array(arr);
  }

  return RESPValue::Null();
}
