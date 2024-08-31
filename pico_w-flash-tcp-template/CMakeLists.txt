cmake_minimum_required(VERSION 3.13)

set(TEST_MODE OFF)

set(PICO_SDK_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/pico-sdk")
set(FREERTOS_DIR ${PICO_SDK_PATH}/lib/tinyusb/lib/FreeRTOS-Kernel)

set(PICO_BOARD "pico_w")
set(WIFI_SSID "hoge")
set(WIFI_PASSWORD "hoge")

set(FAMILY RP2040 CACHE INTERNAL "")
set(FAMILY_MCU RP2040 CACHE INTERNAL "")

set(PROJECT_NAME "pico_w-flash-tcp-template" C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

project(${PROJECT_NAME})

pico_sdk_init()

file(GLOB FREERTOS_DEPENDENCIES ${FREERTOS_DIR}/*.c)

if(TEST_MODE)
    set(MAIN_FILE test_main.c)
else()
    set(MAIN_FILE main.c)
endif()

add_executable(
    ${PROJECT_NAME} 
    ${MAIN_FILE}
    ${FREERTOS_DEPENDENCIES}
    ${FREERTOS_DIR}/portable/MemMang/heap_3.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM0/port.c
)

pico_add_extra_outputs(${PROJECT_NAME})

include_directories(
    "."
    ${CMAKE_CURRENT_LIST_DIR}
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM0
)

target_compile_definitions(${PROJECT_NAME}  PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\" 
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )

target_link_libraries(${PROJECT_NAME} 
    pico_stdlib 
    pico_cyw43_arch 
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_flash
    hardware_adc
    )
