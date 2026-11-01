FROM ubuntu:22.04

# Устанавливаем базовые инструменты + make
RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    git \
    wget \
    gnupg \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем свежий CMake (3.27+)
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
    && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
    && apt-get update \
    && apt-get install -y cmake \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем RISC-V cross-compiler
RUN wget https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2023.10.18/riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.10.18-nightly.tar.gz \
    && tar -xzf riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.10.18-nightly.tar.gz -C /opt \
    && rm riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.10.18-nightly.tar.gz

# Добавляем компилятор в PATH
ENV PATH="/opt/riscv/bin:${PATH}"

# Устанавливаем QEMU для запуска RISC-V бинарников
RUN apt-get update && apt-get install -y qemu-user && rm -rf /var/lib/apt/lists/*

# Рабочая папка
WORKDIR /app