cmake_minimum_required(VERSION 3.2)
project(z80pal)

set(AVR_MCU atmega2560)

FILE(GLOB_RECURSE SRC_LIST "${CMAKE_CURRENT_LIST_DIR}/src/*.*")

include_directories("${CMAKE_CURRENT_LIST_DIR}/include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/src")

SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-Wno-error=narrowing -fno-exceptions -fno-threadsafe-statics -fpermissive -std=gnu++11 -Os -Wall -ffunction-sections -fdata-sections -flto -mmcu=atmega2560")
SET(CMAKE_C_FLAGS_DISTRIBUTION "-std=gnu11 -fno-fat-lto-objects -Os -Wall -ffunction-sections -fdata-sections -flto -mmcu=atmega2560")

SET(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_avr_executable(${PROJECT_NAME} ${SRC_LIST})
