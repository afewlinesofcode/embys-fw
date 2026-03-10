FROM debian:bookworm

# Avoid interactive apt
ENV DEBIAN_FRONTEND=noninteractive
# Optional: speed up rebuilds
ENV CCACHE_DIR=/ccache
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential make ca-certificates \
    ccache \
    gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch libnewlib-arm-none-eabi \
    vim \
  && rm -rf /var/lib/apt/lists/*

# Let make/cmake find toolchain in PATH and use ccache
ENV PATH="/usr/lib/ccache:$PATH"
WORKDIR /work
