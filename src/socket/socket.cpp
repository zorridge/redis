#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

#include "socket.hpp"

SocketRAII::SocketRAII(int fd) : fd(fd) {}
SocketRAII::~SocketRAII()
{
  if (fd >= 0)
    close(fd);
}
SocketRAII::operator int() const { return fd; }
SocketRAII::SocketRAII(SocketRAII &&other) noexcept : fd(other.fd) { other.fd = -1; }
SocketRAII &SocketRAII::operator=(SocketRAII &&other) noexcept
{
  if (this != &other)
  {
    if (fd >= 0)
      close(fd);
    fd = other.fd;
    other.fd = -1;
  }
  return *this;
}

int set_up()
{
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return -1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    close(server_fd);
    return -1;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
  {
    std::cerr << "inet_pton failed for 127.0.0.1\n";
    close(server_fd);
    return -1;
  }
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port " << PORT << "\n";
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, CONNECTION_BACKLOG) != 0)
  {
    std::cerr << "Listen failed\n";
    close(server_fd);
    return -1;
  }

  return server_fd;
}
