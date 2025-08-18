#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

class ClientHandler;

class PubSubManager
{
public:
  bool subscribe(ClientHandler *client, const std::string &channel);
  bool unsubscribe(ClientHandler *client, const std::string &channel);
  size_t unsubscribe_all(ClientHandler *client);
  int publish(const std::string &channel, const std::string &message);

  // Diagnostics
  std::vector<ClientHandler *> subscribers(const std::string &channel) const;
  size_t channel_count() const;

private:
  std::unordered_map<std::string, std::unordered_set<ClientHandler *>> m_channel_subs;
};
