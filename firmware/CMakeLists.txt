# 1. Require a modern CMake version
cmake_minimum_required(VERSION 3.20.0)

# 2. Find Zephyr OS and load the hardware device tree
find_package(Zephyr REQUIRED HINTS {ZEPHYR_BASE})

# 3. Name your specific project
project(smart_appliance)

# 4. Add include directories
target_include_directories(app PRIVATE src/app src/drivers src/telemetry)

# 5. Add source files
target_sources(app PRIVATE 
    src/main.c
    src/app/mixer_sm.c
    src/drivers/slider_gpio.c
    src/drivers/power_btn.c
    src/drivers/motor_pwm.c
    src/telemetry/telemetry.c
)
