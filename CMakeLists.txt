cmake_minimum_required(VERSION 3.10)


include(pico_sdk_import.cmake)

# Set the project name and version
project(Si7021 VERSION 1.0 DESCRIPTION "Si7021 Sensor Library")

# Specify the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# # initialize the Raspberry Pi Pico SDK
pico_sdk_init()


# Create a library named 'si7021' based on the source file
add_library(si7021 STATIC src/si7021.c) # For a static library


# Specify that the library includes header files from the current directory
target_include_directories(si7021 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(si7021 
    pico_stdlib
    hardware_i2c
)

# Specify library's public headers
set_target_properties(si7021 PROPERTIES OUTPUT_NAME si7021)

add_subdirectory(lib/Unity)

add_executable(si7021_test examples/main.c)

target_link_libraries(si7021_test
    si7021
    pico_stdlib
    hardware_i2c
    unity
)

add_executable(si7021_ut test/main.c)

target_link_libraries(si7021_ut
    pico_stdlib
    hardware_i2c
    unity
)

