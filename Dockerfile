FROM public.ecr.aws/ubuntu/ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    libace-dev \
    libmariadb-dev \
    libssl-dev \
    libbz2-dev \
    zlib1g-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/turtlewow/server \
    -DUSE_EXTRACTORS=OFF \
    -DUSE_REALMMERGE=OFF \
    -DUSE_LIBCURL=OFF

RUN cmake --build build --target install -j"$(nproc)"

FROM public.ecr.aws/ubuntu/ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    ca-certificates \
    libace-dev \
    libmariadb-dev \
    libssl3 \
    libbz2-1.0 \
    zlib1g \
    mariadb-client \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/turtlewow/server /opt/turtlewow/server
COPY sql /opt/turtlewow/sql
COPY docker/entrypoint.sh /usr/local/bin/tw-entrypoint.sh

RUN chmod +x /usr/local/bin/tw-entrypoint.sh \
    && mkdir -p /opt/turtlewow/server/data /opt/turtlewow/server/logs /opt/turtlewow/server/honor /opt/turtlewow/server/pdump

WORKDIR /opt/turtlewow/server/bin
ENTRYPOINT ["/usr/local/bin/tw-entrypoint.sh"]
CMD ["mangosd", "-c", "/opt/turtlewow/server/etc/mangosd.conf"]
