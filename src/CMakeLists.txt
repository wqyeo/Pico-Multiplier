cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

project(pico-multiplier C CXX ASM)
set(CMAKE_C_STANDARD 11)

# Initialize the Pico SDK
pico_sdk_init()

# Add your source file to the project
add_executable(main
main.c
)

# Link the Pico SDK libraries to your project
target_link_libraries(main
    pico_stdlib
    pico_time
    hardware_pwm
)
# enable usb output, enable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)