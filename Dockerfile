FROM ubuntu:22.04

# Слой 1: базовые инструменты
RUN apt-get update && apt-get install -y \
    build-essential make git wget gnupg ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Слой 2: cmake
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
    | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' \
    > /etc/apt/sources.list.d/kitware.list \
    && apt-get update && apt-get install -y cmake \
    && rm -rf /var/lib/apt/lists/*

# Слой 3: RISC-V toolchain
RUN wget https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2023.10.18/riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.10.18-nightly.tar.gz \
    && tar -xzf riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.10.18-nightly.tar.gz -C /opt \
    && rm riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.10.18-nightly.tar.gz

ENV PATH="/opt/riscv/bin:${PATH}"

# Слой 4: QEMU + flex/bison (меняется чаще)
RUN apt-get update && apt-get install -y \
    qemu-user m4 flex bison \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app