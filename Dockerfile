# --- Build Stage ---
FROM debian:bullseye AS build

RUN apt-get update && \
  apt-get install -y build-essential cmake git curl zip unzip tar && \
  rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN git clone https://github.com/microsoft/vcpkg.git && \
  ./vcpkg/bootstrap-vcpkg.sh

RUN cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/app/vcpkg/scripts/buildsystems/vcpkg.cmake && \
  cmake --build build

# --- Runtime Stage ---
FROM debian:bullseye

RUN apt-get update && \
  apt-get install -y redis-tools && \
  rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /app/build/server /app/build/server

EXPOSE 6379

CMD ["/app/build/server"]
