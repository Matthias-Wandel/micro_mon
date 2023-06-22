cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico)

include(pico_sdk_import.cmake)


add_compile_options(-mcpu=cortex-m0plus -mthumb)

project(micro_mon_test C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(micro_mon
    main.c
    adc.c
	core1.c
	RP2040-Zero_led.c
)
pico_enable_stdio_usb(micro_mon 1)

target_include_directories(micro_mon PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(micro_mon
    hardware_adc
    pico_stdlib
    pico_multicore
)

pico_add_extra_outputs(micro_mon)