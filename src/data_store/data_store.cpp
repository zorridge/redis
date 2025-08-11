#include "data_store.hpp"

bool DataStore::is_expired(const Entry &entry) const
{
  auto now = std::chrono::steady_clock::now();
  return entry.expire_at != std::chrono::steady_clock::time_point::max() && now > entry.expire_at;
}

RESPValue DataStore::type(const std::string &key)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::SimpleString("none");
  if (std::holds_alternative<RedisString>(it->second.value))
    return RESPValue::SimpleString("string");
  if (std::holds_alternative<RedisList>(it->second.value))
    return RESPValue::SimpleString("list");
  if (std::holds_alternative<RedisStream>(it->second.value))
    return RESPValue::SimpleString("stream");

  return RESPValue::SimpleString("none");
}

RESPValue DataStore::set(const std::string &key,
                         const std::string &value,
                         std::chrono::milliseconds ttl)
{
  Entry entry;
  entry.value = value;
  if (ttl.count() > 0)
    entry.expire_at = std::chrono::steady_clock::now() + ttl;
  else
    entry.expire_at = std::chrono::steady_clock::time_point::max();
  m_store[key] = std::move(entry);

  return RESPValue::SimpleString("OK");
}

RESPValue DataStore::get(const std::string &key)
{
  auto it = m_store.find(key);
  if (it == m_store.end())
    return RESPValue::Null();

  if (is_expired(it->second))
  {
    m_store.erase(it);
    return RESPValue::Null();
  }

  if (auto *str = std::get_if<RedisString>(&it->second.value))
  {
    return RESPValue::BulkString(*str);
  }
  else if (auto *deq = std::get_if<RedisList>(&it->second.value))
  {
    std::vector<RESPValue> arr;
    for (const auto &s : *deq)
      arr.push_back(RESPValue::BulkString(s));
    return RESPValue::Array(std::move(arr));
  }

  return RESPValue::Null();
}

RESPValue DataStore::incr(const std::string &key)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
  {
    if (it != m_store.end())
      m_store.erase(it);

    m_store[key] = {"1", std::chrono::steady_clock::time_point::max()};
    return RESPValue::Integer(1);
  }

  Entry &entry = it->second;
  if (auto *str = std::get_if<RedisString>(&entry.value))
  {
    int64_t current_val;
    try
    {
      current_val = std::stoll(*str);
    }
    catch (...)
    {
      return RESPValue::Error("ERR value is not an integer or out of range");
    }

    int64_t new_val = current_val + 1;
    entry.value = std::to_string(new_val);
    return RESPValue::Integer(new_val);
  }
  else
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
}
