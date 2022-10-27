cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(my_pico_test C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(my_pico
    main.c
    adc.c
    wifi_scan.c
    tcp_server.c
)
pico_enable_stdio_usb(my_pico 1)

target_include_directories(my_pico PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(my_pico
	hardware_adc
	pico_cyw43_arch_lwip_poll
	pico_stdlib
)

pico_add_extra_outputs(my_pico)