cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_TOOLCHAIN_FILE cmake/toolchain-arm-none-eabi-gcc.cmake)
project(blink VERSION 0.1.0 LANGUAGES CXX C ASM)

include(cmake/stm32f765.cmake)

add_executable(blink src/usb_cdc.c src/main.c)
target_link_libraries(blink stm32f103)
set_property(TARGET blink PROPERTY C_STANDARD 11)
target_include_directories(blink PRIVATE src/include)
stm32_add_linker_script(blink PRIVATE src/ld/basic.stm32.ld)
stm32_add_flash_targets(blink)
#
#add_executable(timer src/timer.cpp)
#target_link_libraries(timer stm32f765)
#set_property(TARGET timer PROPERTY CXX_STANDARD 17)
#stm32_add_linker_script(timer PRIVATE src/stm32f1.ld)
#stm32_add_flash_targets(timer)