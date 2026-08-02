FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive
ENV CCACHE_DIR=/ccache

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  make \
  ca-certificates \
  ccache \
  gcc-arm-none-eabi \
  binutils-arm-none-eabi \
  gdb-multiarch \
  libnewlib-arm-none-eabi \
  libstdc++-arm-none-eabi-newlib \
  vim \
  && rm -rf /var/lib/apt/lists/*

ENV PATH="/usr/lib/ccache:$PATH"

WORKDIR /work
