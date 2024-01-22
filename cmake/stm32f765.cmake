#include(FetchContent)
#FetchContent_Declare(libopencm3
#  GIT_REPOSITORY https://github.com/libopencm3/libopencm3
#  GIT_TAG master
#)
#FetchContent_MakeAvailable(libopencm3)
#FetchContent_GetProperties(libopencm3)

set(libopencm3_SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/libopencm3")

# create a target to build libopencm3 -- only for the target we need
add_custom_target(libopencm3 make TARGETS=stm32/f4 WORKING_DIRECTORY ${libopencm3_SOURCE_DIR})

# Create a specific CPU target with the appropriate options etc
add_library(stm32f103 STATIC IMPORTED)
set_property(TARGET stm32f103 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${libopencm3_SOURCE_DIR}/include)
set_property(TARGET stm32f103 PROPERTY IMPORTED_LOCATION ${libopencm3_SOURCE_DIR}/lib/libopencm3_stm32f4.a)
add_dependencies(stm32f103 libopencm3)
target_link_directories(stm32f103 INTERFACE ${libopencm3_SOURCE_DIR}/lib)

target_compile_definitions(stm32f103 INTERFACE -DSTM32F4)

set(COMPILE_OPTIONS 
  --static
  -nostartfiles
  -fno-common
  -mcpu=cortex-m4
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
)
target_compile_options(stm32f103 INTERFACE ${COMPILE_OPTIONS})
target_link_options(stm32f103 INTERFACE ${COMPILE_OPTIONS})


#function(stm32_add_flash_targets TARGET)
#    add_custom_target(${TARGET}-stlink-flash
#      bash -c "openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg \
#                -f /usr/share/openocd/scripts/target/stm32f7x.cfg \
#                -c 'reset_config none; program ${TARGET}.elf verify reset exit'"
#      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#      DEPENDS ${TARGET}
#      VERBATIM
#    )
#endfunction()