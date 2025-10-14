cmake_minimum_required(VERSION 3.13)

# Set board type to Pico 2 W (which has WiFi and RP2350 chip)
set(PICO_BOARD pico2_w)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_udp_server C CXX ASM)
pico_sdk_init()

add_executable(pico_udp_server
    main.c
    hid_server.c
    usb_descriptors.c
)

# Add current directory to include path for lwipopts.h
target_include_directories(pico_udp_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_udp_server
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_usb(pico_udp_server 0)
pico_enable_stdio_uart(pico_udp_server 1)

pico_add_extra_outputs(pico_udp_server)
