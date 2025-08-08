#pragma once

struct SocketRAII
{
public:
  explicit SocketRAII(int fd = -1);
  ~SocketRAII();

  // Disable copy
  SocketRAII(const SocketRAII &) = delete;
  SocketRAII &operator=(const SocketRAII &) = delete;

  // Enable move
  SocketRAII(SocketRAII &&other) noexcept;
  SocketRAII &operator=(SocketRAII &&other) noexcept;

  int get() const { return fd; }
  operator int() const;

private:
  int fd;
};

int set_up();
