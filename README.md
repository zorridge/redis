# Redis

> ... but homebrewed (and _slightly_ more fragile) in C++.

## ✨ Features

> If you squint, it’s almost Redis.

### Commands That (Probably) Work

#### Strings
* `GET`
* `SET`

#### Lists
* `LPUSH`
* `RPUSH`
* `LPOP`
* `LLEN`
* `LRANGE`
* `BLPOP` (using conditional variables)

## 🚧 Setup

> The part everyone skips anyway.

```bash
# 1. Install vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh

export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH

# 2. Start server
./run.sh

# 3. Connect client
redis-cli -h localhost -p 6379
```

> Yes, I am reinventing the wheel. But at least it’s not in JavaScript.

