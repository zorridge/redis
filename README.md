# Redis

> ... but homebrewed (and _slightly_ more fragile) in C++.

A project focused on emulating Redis’s high efficiency and performance, utilizing a single-threaded, event-driven architecture with I/O multiplexing (kqueue/epoll) to support concurrent client connections and blocking operations.

```mermaid
flowchart TB
    %% Define Nodes with Appropriate Shapes
    A@{ shape: processes, label: "Clients" }
    B@{ shape: lean-r, label: "TCP Network Layer" }
    C@{ shape: rounded, label: "I/O Multiplexing\n(kqueue/epoll)" }
    D@{ shape: rounded, label: "Main Event Loop" }
    E@{ shape: rounded, label: "Command Dispatcher" }
    F@{ shape: database, label: "Data Store" }
    G@{ shape: rounded, label: "Blocking Manager" }

    %% Request Lifecycle
    A <-- Request/Response --> B
    B -- "New Event" --> C
    C -- "Ready To Read" --> D
    D -- "Parse & Execute" --> E
    E -- "Read/Write Data" --> F
    E -- "Block/Unblock Client" --> G

    %% Response Lifecycle
    G -- "Client Ready" --> D
    D -- "Serialize Response" --> B

    %% Highlighting Core Components
    classDef core fill:#00897b,stroke-width:2px,color:#fff;
    D:::core
```

## ✨ Features

> If you squint, it’s almost Redis.

### Commands That (Probably) Work

#### Strings
> No frills, just vibes.
* `GET`
* `SET`

#### Lists
> Engineered for O(1) at the ends. Engineered for O(¯\\(ツ)/¯) in the middle.
* `LPUSH`
* `RPUSH`
* `LPOP`
* `LLEN`
* `LRANGE`
* `BLPOP`

#### Streams
> Wait, Redis can do that?
* `XADD`
* `XRANGE`
* `XREAD`

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




