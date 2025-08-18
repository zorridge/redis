#include "pubsub_manager.hpp"
#include "../client/client_handler.hpp"

#include <algorithm>

bool PubSubManager::subscribe(ClientHandler *client, const std::string &channel)
{
  auto &set = m_channel_subs[channel];
  auto [it, inserted] = set.insert(client);
  return inserted;
}

bool PubSubManager::unsubscribe(ClientHandler *client, const std::string &channel)
{
  auto it = m_channel_subs.find(channel);
  if (it == m_channel_subs.end())
    return false;

  auto &set = it->second;
  size_t erased = set.erase(client);
  if (set.empty())
    m_channel_subs.erase(it);

  return erased > 0;
}

size_t PubSubManager::unsubscribe_all(ClientHandler *client)
{
  size_t removed = 0;
  std::vector<std::string> empty_channels;
  empty_channels.reserve(m_channel_subs.size());

  for (auto &kv : m_channel_subs)
  {
    auto &set = kv.second;
    removed += set.erase(client);
    if (set.empty())
      empty_channels.push_back(kv.first);
  }

  for (const auto &ch : empty_channels)
    m_channel_subs.erase(ch);

  return removed;
}

int PubSubManager::publish(const std::string &channel, const std::string &message)
{
  auto it = m_channel_subs.find(channel);
  if (it == m_channel_subs.end())
    return 0;

  std::vector<ClientHandler *> clients(it->second.begin(), it->second.end());
  for (ClientHandler *client : clients)
  {
    client->send_pubsub_message("message", channel, RESPValue::BulkString(message));
  }

  return static_cast<int>(clients.size());
}

std::vector<ClientHandler *> PubSubManager::subscribers(const std::string &channel) const
{
  std::vector<ClientHandler *> out;
  auto it = m_channel_subs.find(channel);
  if (it == m_channel_subs.end())
    return out;
  out.reserve(it->second.size());
  for (ClientHandler *client : it->second)
    out.push_back(client);
  return out;
}

size_t PubSubManager::channel_count() const
{
  return m_channel_subs.size();
}
