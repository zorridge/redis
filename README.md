# Redis

[![Web Terminal](https://img.shields.io/badge/Web%20Terminal-redis.ziheng.dev-ff4438?style=for-the-badge)](https://redis.ziheng.dev/)

> ... but homebrewed (and _slightly_ more fragile) in C++.

A project focused on emulating Redis’s high efficiency and performance, utilizing a single-threaded, event-driven architecture with I/O multiplexing (kqueue/epoll) to support concurrent client connections and blocking operations.

```mermaid
flowchart TB
    clients@{ shape: processes, label: "Clients" }
    tcp_layer@{ shape: lean-r, label: "TCP Network Layer" }
    io_multiplexing@{ shape: rounded, label: "I/O Multiplexing\n(kqueue/epoll)" }
    main_event_loop@{ shape: rounded, label: "Main Event Loop" }
    command_dispatcher@{ shape: rounded, label: "Command Dispatcher" }
    client_buffers@{ shape: cylinder, label: "Client Buffers" }
    data_store@{ shape: database, label: "Data Store" }

    subgraph aux["Auxiliary Components"]
        direction TB
        pubsub_manager@{ shape: rounded, label: "Pub/Sub Manager" }
        blocking_manager@{ shape: rounded, label: "Blocking Manager" }
        client_tx_queue@{ shape: rounded, label: "Client Transaction Queue" }
    end

    %% Request Lifecycle
    clients <-- Request/Response --> tcp_layer
    tcp_layer -- "Mark Readable" --> io_multiplexing
    io_multiplexing -- "New Event" --> main_event_loop
    main_event_loop -- "Parse & Execute" --> command_dispatcher
    command_dispatcher -- "Queue Response" --> client_buffers
    command_dispatcher -- "Read/Write Data" --> data_store
    command_dispatcher -- "Access" --> aux
    pubsub_manager -- "Publish Message" --> client_buffers


    %% Response Lifecycle
    main_event_loop -- "Flush Buffers" --> tcp_layer

    classDef core fill:#00574b,stroke-width:2px,color:#fff;
    main_event_loop:::core

    classDef auxiliary fill:#00897b,stroke-width:1px,color:#fff;
    pubsub_manager:::auxiliary
    client_tx_queue:::auxiliary
    blocking_manager:::auxiliary
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

#### Pub/Sub
> I also homebrewed Kafka.
* `SUBSCRIBE`
* `UNSUBSCRIBE`
* `PUBLISH`

#### Transactions
> For rollback, please consult your nearest relational database.
* `MULTI`
* `EXEC`
* `DISCARD`

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









