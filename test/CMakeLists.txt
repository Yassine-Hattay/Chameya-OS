cmake_minimum_required(VERSION 3.5)
project(unit_tests)

# Include directories for headers
include_directories(
	"."
    "./SPI"  
    "../components/UART"
    "../components/SPI"
    "./debugging"
    "../components/debugging"
    "../../ESP8266_RTOS_SDK/components/esp_common/include"
)

# Source files
set(SOURCE_FILES
    "./main_test.c"
    "./SPI/SPI_test.c"  
    "../components/SPI/SPI.c"
    "./debugging/my_print_test.c"  
    "../components/debugging/my_print.c"
    "./unity.c" 
)

# Create the executable
add_executable(unit_tests.exe ${SOURCE_FILES})

