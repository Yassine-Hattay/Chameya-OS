cmake_minimum_required(VERSION 3.5)
project(unit_tests)

# Include directories for headers
include_directories(
    "."  
    "../components/UART"
    "../../ESP8266_RTOS_SDK/components/esp_common/include"
)

# Source files
set(SOURCE_FILES
    "./main_test.c"
    "./UART_tests.c"
    "./unity.c" 
    "../components/UART/recive_uart.c"
)

# Create the executable
add_executable(unit_tests.exe ${SOURCE_FILES})

