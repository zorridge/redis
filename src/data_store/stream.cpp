#include "data_store.hpp"

bool parse_stream_id(const std::string &id_str,
                     StreamID &id,
                     const StreamID *last_id = nullptr,
                     bool is_range_end = false);

RESPValue DataStore::xadd(const std::string &key,
                          const std::string &id_str,
                          const StreamEntry &entry)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Try to find existing stream and get last_id if possible
  StreamID id;
  const StreamID *last_id = nullptr;
  RedisStream *stream = nullptr;

  auto it = m_store.find(key);
  if (it != m_store.end() && !is_expired(it->second))
  {
    stream = std::get_if<RedisStream>(&it->second.value);
    if (!stream)
      return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");
    if (!stream->empty())
      last_id = &stream->rbegin()->first;
  }

  if (!parse_stream_id(id_str, id, last_id))
    return RESPValue::Error("ERR Invalid stream ID specified as stream command argument");

  if (id.ms == 0 && id.seq == 0)
    return RESPValue::Error("ERR The ID specified in XADD must be greater than 0-0");

  if (last_id && id <= (*last_id))
    return RESPValue::Error("ERR The ID specified in XADD is equal or smaller than the target stream top item");

  if (!stream)
  {
    RedisStream new_stream;
    new_stream[id] = entry;
    m_store.insert_or_assign(key, Entry{std::move(new_stream),
                                        std::chrono::steady_clock::time_point::max()});
  }
  else
    (*stream)[id] = entry;

  return RESPValue::BulkString(id.to_string());
}

RESPValue DataStore::xrange(const std::string &key,
                            const std::string &start_id_str,
                            const std::string &end_id_str,
                            int64_t count)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_store.find(key);
  if (it == m_store.end() || is_expired(it->second))
    return RESPValue::Array({});

  RedisStream *stream = std::get_if<RedisStream>(&it->second.value);
  if (!stream)
    return RESPValue::Error("WRONGTYPE Operation against a key holding the wrong kind of value");

  StreamID start_id, end_id;
  if (!parse_stream_id(start_id_str, start_id, nullptr, false) ||
      !parse_stream_id(end_id_str, end_id, nullptr, true))
    return RESPValue::Error("ERR Invalid stream ID specified as stream command argument");

  std::vector<RESPValue> results;
  auto iter = stream->lower_bound(start_id);
  for (; iter != stream->end(); ++iter)
  {
    if (iter->first > end_id)
      break;

    // Add [id, [field1, value1, ...]]
    std::vector<RESPValue> entry_fields;
    for (const auto &s : iter->second)
      entry_fields.push_back(RESPValue::BulkString(s));

    results.push_back(RESPValue::Array({RESPValue::BulkString(iter->first.to_string()),
                                        RESPValue::Array(entry_fields)}));

    if (count > 0 && static_cast<int64_t>(results.size()) >= count)
      break;
  }

  return RESPValue::Array(results);
}

bool parse_stream_id(const std::string &id_str,
                     StreamID &id,
                     const StreamID *last_id,
                     bool is_range_end)
{
  using namespace std::chrono;

  if (id_str == "*")
  {
    id.ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (last_id && id.ms == last_id->ms)
      id.seq = last_id->seq + 1;
    else
      id.seq = 0;
    return true;
  }

  size_t dash = id_str.find('-');
  std::string ms_part, seq_part;
  if (dash == std::string::npos)
  {
    ms_part = id_str;
    seq_part = is_range_end ? std::to_string(UINT64_MAX) : "0";
  }
  else
  {
    ms_part = id_str.substr(0, dash);
    seq_part = id_str.substr(dash + 1);
  }

  // Check for negative numbers
  if ((!ms_part.empty() && ms_part[0] == '-') || (!seq_part.empty() && seq_part[0] == '-'))
    return false;

  try
  {
    id.ms = std::stoull(ms_part);
    if (seq_part == "*")
    {
      if (last_id && id.ms == last_id->ms)
        id.seq = last_id->seq + 1;
      else
        id.seq = id.ms == 0 ? 1 : 0;
    }
    else
      id.seq = std::stoull(seq_part);
  }
  catch (...)
  {
    return false;
  }

  return true;
}
