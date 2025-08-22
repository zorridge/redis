# --- Server build stage ---
FROM debian:bullseye AS build-server

RUN apt-get update && \
  apt-get install -y build-essential cmake git curl zip unzip tar && \
  rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN git clone https://github.com/microsoft/vcpkg.git && \
  ./vcpkg/bootstrap-vcpkg.sh

RUN cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/app/vcpkg/scripts/buildsystems/vcpkg.cmake && \
  cmake --build build

# --- redis-cli 8.x build stage ---
FROM debian:bullseye AS build-cli

RUN apt-get update && \
  apt-get install -y build-essential curl make tcl && \
  rm -rf /var/lib/apt/lists/*

WORKDIR /src

RUN curl -O https://download.redis.io/releases/redis-8.2.0.tar.gz && \
  tar xzf redis-8.2.0.tar.gz && \
  cd redis-8.2.0 && make redis-cli

# --- Runtime stage ---
FROM debian:bullseye
WORKDIR /app

# Copy server
COPY --from=build-server /app/build/server /app/build/server

# Copy redis-cli 8.x
COPY --from=build-cli /src/redis-8.2.0/src/redis-cli /usr/local/bin/redis-cli

EXPOSE 6379

CMD ["/app/build/server"]
