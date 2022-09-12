# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/kosmas/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PicoKenbak C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(PicoKenbak PicoKenbak.c processor.c processor.h
        )

pico_set_program_name(PicoKenbak "PicoKenbak")
pico_set_program_version(PicoKenbak "0.1")

pico_enable_stdio_uart(PicoKenbak 0)
pico_enable_stdio_usb(PicoKenbak 1)

# Add the standard library to the build
target_link_libraries(PicoKenbak pico_stdlib)

pico_add_extra_outputs(PicoKenbak)

