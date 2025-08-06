#include "data_store.hpp"

bool parse_stream_id(const std::string &id_str, StreamID &id);

RESPValue DataStore::xadd(const std::string &key,
                          const std::string &id_str,
                          const StreamEntry &entry)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  StreamID id;
  if (!parse_stream_id(id_str, id))
    return RESPValue::Error("ERR Invalid stream ID specified as stream command argument");

  if (id.ms <= 0 && id.seq <= 0)
    return RESPValue::Error("ERR The ID specified in XADD must be greater than 0-0");

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
  {
    RedisStream stream;
    stream[id] = entry;

    m_store.insert_or_assign(key, Entry{std::move(stream),
                                        std::chrono::steady_clock::time_point::max()});
  }
  else if (auto *stream = std::get_if<RedisStream>(&it->second.value))
  {
    // Validate ID is strictly greater than last entry
    if (!stream->empty())
    {
      const StreamID &last_id = stream->rbegin()->first;
      if (id.ms < last_id.ms || id.seq <= last_id.seq)
        return RESPValue::Error("ERR The ID specified in XADD is equal or smaller than the target stream top item");
    }

    (*stream)[id] = entry;
  }
  else
  {
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
  }

  return RESPValue::SimpleString(id.to_string());
}

bool DataStore::parse_stream_id(const std::string &id_str, StreamID &id)
{
  size_t dash = id_str.find('-');
  if (dash == std::string::npos)
    return false;

  try
  {
    id.ms = std::stoull(id_str.substr(0, dash));
    id.seq = std::stoull(id_str.substr(dash + 1));
  }
  catch (...)
  {
    return false;
  }
  return true;
}
