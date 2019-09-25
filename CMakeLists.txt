cmake_minimum_required(VERSION 3.2)
project(z80pal C)

# include(CMakeListsPrivate.txt)

set(AVR_MCU atmega2560)

FILE(GLOB_RECURSE SRC_LIST "${CMAKE_CURRENT_LIST_DIR}/src/*.*" "${CMAKE_CURRENT_LIST_DIR}/lib/*.*" "${CMAKE_CURRENT_LIST_DIR}/.pio/libdeps/megaatmega2560/*.*")

add_avr_executable(${PROJECT_NAME} ${SRC_LIST})
