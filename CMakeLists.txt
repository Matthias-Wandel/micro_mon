cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(sens_remote_test C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(sens_remote
    main.c
    adc.c
    wifi_scan.c
    tcp_server.c
	one_wire.cpp
	one_wire_18b20.cpp
)
pico_enable_stdio_usb(sens_remote 1)

target_include_directories(sens_remote PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(sens_remote
	hardware_adc
	pico_cyw43_arch_lwip_poll
	pico_stdlib
    pico_multicore
)

pico_add_extra_outputs(sens_remote)