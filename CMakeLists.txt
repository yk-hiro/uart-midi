cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(uart-midi)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(uart-midi
    uart_midi_test.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(uart-midi pico_stdlib)

# enable output = 1, disable output = 0
pico_enable_stdio_usb(uart-midi 1)
pico_enable_stdio_uart(uart-midi 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(uart-midi)
