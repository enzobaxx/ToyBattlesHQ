# syntax=docker/dockerfile:1

FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential gcc-14 g++-14 cmake ninja-build git curl zip unzip tar \
        pkg-config python3 autoconf automake autoconf-archive libtool bison flex ca-certificates \
    && rm -rf /var/lib/apt/lists/*
ENV CC=gcc-14 CXX=g++-14
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg
WORKDIR /src
COPY . /src
RUN cmake -B build -S . -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && cmake --build build --config Release

FROM ubuntu:24.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        mariadb-server mariadb-client supervisor ca-certificates bash procps \
    && rm -rf /var/lib/apt/lists/*
RUN mkdir -p /app/emu/Output /app/emu/Setup /run/mysqld \
    && chown -R mysql:mysql /run/mysqld /var/lib/mysql
COPY --from=builder /src/AuthServer.elf /app/emu/Output/
COPY --from=builder /src/MainServer.elf /app/emu/Output/
COPY --from=builder /src/CastServer.elf /app/emu/Output/
COPY --from=builder /src/ExternalLibraries/cgd_original /app/emu/ExternalLibraries/cgd_original
COPY --from=builder /src/RewardItemIDs.txt /app/emu/RewardItemIDs.txt
COPY --from=builder /src/Setup/config.ini /app/emu/Setup/config.ini
COPY --from=builder /src/microvolts-db.sql /app/emu/microvolts-db.sql
COPY deploy/docker/entrypoint.sh /app/entrypoint.sh
COPY deploy/docker/dbinit.sh /app/dbinit.sh
COPY deploy/docker/run-server.sh /app/run-server.sh
COPY deploy/docker/supervisord.conf /app/supervisord.conf
RUN chmod +x /app/entrypoint.sh /app/dbinit.sh /app/run-server.sh /app/emu/Output/*.elf
ENV MV_DB_PORT=3305
ENTRYPOINT ["/app/entrypoint.sh"]
