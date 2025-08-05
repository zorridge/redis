#include "data_store.hpp"

std::shared_ptr<std::condition_variable> DataStore::get_cv_for_key(const std::string &key)
{
  auto it = m_key_cvs.find(key);
  if (it == m_key_cvs.end())
  {
    auto cv = std::make_shared<std::condition_variable>();
    m_key_cvs[key] = cv;
    return cv;
  }
  return it->second;
}

void DataStore::notify_list_push(const std::string &key)
{
  auto cv = get_cv_for_key(key);
  cv->notify_all();
}

RESPValue DataStore::llen(const std::string &key)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Integer(0);

  if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
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
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
  {
    // Key doesn't exist or expired: create new list
    m_store[key] = Entry{std::deque<std::string>(values.begin(), values.end()),
                         std::chrono::steady_clock::time_point::max()};

    notify_list_push(key); // Notify waiters: list is now non-empty
    return RESPValue::Integer(values.size());
  }

  if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
  {
    bool was_empty = deq->empty();
    deq->insert(deq->end(), values.begin(), values.end());

    if (was_empty && !deq->empty())
      notify_list_push(key); // Notify waiters only if list transitioned from empty to non-empty

    return RESPValue::Integer(deq->size());
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::lpush(const std::string &key, const std::vector<std::string> &values)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
  {
    // Key doesn't exist or expired: create new list
    m_store[key] = Entry{std::deque<std::string>(values.rbegin(), values.rend()),
                         std::chrono::steady_clock::time_point::max()};

    notify_list_push(key); // Notify waiters: list is now non-empty
    return RESPValue::Integer(values.size());
  }

  if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
  {
    bool was_empty = deq->empty();
    deq->insert(deq->begin(), values.rbegin(), values.rend());

    if (was_empty && !deq->empty())
      notify_list_push(key); // Notify waiters only if list transitioned from empty to non-empty

    return RESPValue::Integer(deq->size());
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::lrange(const std::string &key, int64_t start, int64_t stop)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Array({});

  if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
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
    return RESPValue::Array(result);
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::lpop(const std::string &key, int64_t count)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Null();

  if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
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
      return RESPValue::Array(popped);
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
}

RESPValue DataStore::blpop(const std::string &key, double timeout_seconds)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  auto pop_if_available = [&]() -> std::optional<std::string>
  {
    auto it = m_store.find(key);
    if (it != m_store.end() && !is_expired(it->second))
    {
      if (auto *deq = std::get_if<std::deque<std::string>>(&it->second.value))
      {
        if (!deq->empty())
        {
          std::string val = std::move(deq->front());
          deq->pop_front();
          if (deq->empty())
            m_store.erase(it);
          return val;
        }
      }
    }
    return std::nullopt;
  };

  // Try to pop immediately
  if (auto val = pop_if_available())
  {
    return RESPValue::Array({RESPValue::BulkString(key), RESPValue::BulkString(*val)});
  }

  // Otherwise, wait up to timeout_seconds
  auto cv = get_cv_for_key(key);
  if (timeout_seconds == 0.0)
  {
    // Wait indefinitely
    while (true)
    {
      cv->wait(lock);
      if (auto val = pop_if_available())
        return RESPValue::Array({RESPValue::BulkString(key), RESPValue::BulkString(*val)});
    }
  }
  else
  {
    // Wait with timeout
    auto timeout = std::chrono::steady_clock::now() +
                   std::chrono::duration<double>(timeout_seconds);
    while (true)
    {
      if (cv->wait_until(lock, timeout) == std::cv_status::timeout)
        break;
      if (auto val = pop_if_available())
        return RESPValue::Array({RESPValue::BulkString(key), RESPValue::BulkString(*val)});
    }
  }

  // Timeout
  return RESPValue::Null();
}
