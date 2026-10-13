# Base image containing Zephyr RTOS v3.7.0, ARM SDK 0.16.8, and QEMU
FROM zephyr-base:latest

USER root
WORKDIR /usr/src/app

# 1. Install Renode runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgtk-3-0 libglib2.0-0 uml-utilities screen wget ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# 2. Download and install Antmicro Renode Portable (62 MB from builds.renode.io)
RUN mkdir -p /opt/renode && \
    cd /opt && \
    wget --tries=5 --timeout=60 https://builds.renode.io/renode-latest.linux-portable.tar.gz && \
    tar -xf renode-latest.linux-portable.tar.gz -C /opt/renode --strip-components=1 && \
    rm renode-latest.linux-portable.tar.gz

# 3. Add Renode to system PATH
ENV PATH="/opt/renode:/opt/zephyr-sdk-0.16.8/sysroots/x86_64-pokysdk-linux/usr/bin:/opt/venv/bin:$PATH"