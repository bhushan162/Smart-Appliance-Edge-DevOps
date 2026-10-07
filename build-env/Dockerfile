# 1. Define the base image environment
FROM ubuntu:24.04

# 2. Establish the isolated application folder
WORKDIR /usr/src/app

# Prevent apt from prompting for user input (like timezone selection) which hangs Docker builds
ARG DEBIAN_FRONTEND=noninteractive

# 3. Install system dependencies (Chained correctly with && \)
RUN apt-get update && apt-get install -y --no-install-recommends \
    git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget \
    python3-dev python3-venv python3-tk xz-utils file make gcc gcc-multilib \
    g++-multilib libsdl2-dev libmagic1

# 4. Create the virtual environment in a central system directory
RUN python3 -m venv /opt/venv

# 5. Prepend the venv to PATH so all subsequent RUN and CMD calls use it automatically
ENV PATH="/opt/venv/bin:$PATH"

# 6. Install West
RUN pip install --upgrade pip && \
    pip install west

# 7. Initialize Zephyr workspace, update modules, and install the Zephyr SDK
# Notice how every line ends with '&& \' to ensure if one command fails, the whole build fails cleanly.
# We also specify '--toolchains arm-zephyr-eabi' to save ~10GB of space by not installing x86/RISC-V/etc.
RUN west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.7.0 /opt/zephyrproject && \
    cd /opt/zephyrproject && \
    west update && \
    pip install -r zephyr/scripts/requirements.txt && \
    west zephyr-export 

# 8. Download and install the Zephyr SDK manually for CI stability
RUN cd /opt && \
    wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64.tar.xz && \
    tar -xf zephyr-sdk-0.16.8_linux-x86_64.tar.xz && \
    rm zephyr-sdk-0.16.8_linux-x86_64.tar.xz && \
    cd zephyr-sdk-0.16.8 && \
    ./setup.sh -t arm-zephyr-eabi -h -c


# 9. Set permanent environment variables for West, Zephyr, and QEMU
ENV ZEPHYR_BASE=/opt/zephyrproject/zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.16.8
ENV PATH="/opt/zephyr-sdk-0.16.8/sysroots/x86_64-pokysdk-linux/usr/bin:/opt/venv/bin:$PATH"
