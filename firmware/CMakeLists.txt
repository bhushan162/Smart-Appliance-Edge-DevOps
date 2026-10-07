# 1. Require a modern CMake version
cmake_minimum_required(VERSION 3.20.0)

# 2. This is the magic line. It finds the Zephyr OS, loads the hardware device tree 
# for the STM32F401, and initializes the cross-compiler toolchain.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# 3. Name your specific project
project(smart_appliance)

# 4. Add your source files to Zephyr's built-in 'app' target
target_sources(app PRIVATE src/main.c)