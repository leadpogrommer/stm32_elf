cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_TOOLCHAIN_FILE cmake/toolchain-arm-none-eabi-gcc.cmake)
project(blink VERSION 0.1.0 LANGUAGES CXX C ASM)

include(cmake/stm32f765.cmake)

add_executable(blink
        src/usb_request.c src/main.c
        src/opencm3_freertos.c
        thirdparty/freertos/portable/GCC/ARM_CM3/port.c
        thirdparty/freertos/list.c
        thirdparty/freertos/tasks.c
        src/fs.c
        src/usb_fs.c
        )
target_link_libraries(blink stm32f103)
set_property(TARGET blink PROPERTY C_STANDARD 11)
target_include_directories(blink PRIVATE src/include thirdparty/freertos/include thirdparty/freertos/portable/GCC/ARM_CM3)
target_compile_options(blink PRIVATE "-specs=nano.specs")
target_link_options(blink PRIVATE "-specs=nano.specs")
stm32_add_linker_script(blink PRIVATE src/ld/basic.stm32.ld)
stm32_add_flash_targets(blink)
#
#add_executable(timer src/timer.cpp)
#target_link_libraries(timer stm32f765)
#set_property(TARGET timer PROPERTY CXX_STANDARD 17)
#stm32_add_linker_script(timer PRIVATE src/stm32f1.ld)
#stm32_add_flash_targets(timer)