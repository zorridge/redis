#include "data_store.hpp"

RESPValue DataStore::llen(const std::string &key)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Integer(0);

  if (auto *deq = std::get_if<RedisList>(&it->second.value))
  {
    return RESPValue::Integer(deq->size());
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::rpush(const std::string &key, const std::vector<std::string> &values)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
  {
    // Key doesn't exist or expired: create new list
    m_store.insert_or_assign(key, Entry(RedisList{values.begin(), values.end()},
                                        std::chrono::steady_clock::time_point::max()));
    return RESPValue::Integer(values.size());
  }

  if (auto *deq = std::get_if<RedisList>(&it->second.value))
  {
    deq->insert(deq->end(), values.begin(), values.end());
    return RESPValue::Integer(deq->size());
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::lpush(const std::string &key, const std::vector<std::string> &values)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
  {
    // Key doesn't exist or expired: create new list
    m_store.insert_or_assign(key, Entry(RedisList{values.rbegin(), values.rend()},
                                        std::chrono::steady_clock::time_point::max()));
    return RESPValue::Integer(values.size());
  }

  if (auto *deq = std::get_if<RedisList>(&it->second.value))
  {
    deq->insert(deq->begin(), values.rbegin(), values.rend());
    return RESPValue::Integer(deq->size());
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::lrange(const std::string &key, int64_t start, int64_t stop)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Array({});

  if (auto *deq = std::get_if<RedisList>(&it->second.value))
  {
    int64_t len = static_cast<int64_t>(deq->size());
    if (start < 0)
      start = len + start;
    if (stop < 0)
      stop = len + stop;
    if (start < 0)
      start = 0;
    if (stop < 0)
      stop = 0;
    if (stop >= len)
      stop = len - 1;

    std::vector<RESPValue> result;
    for (int64_t i = start; i <= stop && i < len; ++i)
      result.push_back(RESPValue::BulkString((*deq)[i]));
    return RESPValue::Array(std::move(result));
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::lpop(const std::string &key, int64_t count)
{
  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Null();

  if (auto *deq = std::get_if<RedisList>(&it->second.value))
  {
    // Clamp count to list size
    int64_t actual = std::min<int64_t>(count, deq->size());
    std::vector<RESPValue> popped;
    for (int64_t i = 0; i < actual; ++i)
    {
      popped.push_back(RESPValue::BulkString(deq->front()));
      deq->pop_front();
    }

    // If list is empty after pop, remove the key
    if (deq->empty())
      m_store.erase(it);

    // If count == 1, return single value
    if (count == 1)
      return popped[0];
    else
      return RESPValue::Array(std::move(popped));
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::blpop(const std::string &key)
{
  auto it = m_store.find(key);
  if (it != m_store.end() && !is_expired(it->second))
  {
    if (auto *deq = std::get_if<RedisList>(&it->second.value))
    {
      if (!deq->empty())
      {
        std::string val = std::move(deq->front());
        deq->pop_front();
        if (deq->empty())
          m_store.erase(it);

        return RESPValue::Array({RESPValue::BulkString(key), RESPValue::BulkString(val)});
      }
    }
  }

  return RESP_BLOCK_CLIENT;
}
