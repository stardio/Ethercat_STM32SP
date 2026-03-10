cmake_minimum_required(VERSION 3.22)

project(LCD_Test_Boot C ASM)

# Source files
set(BOOT_SOURCES
    Core/Src/main.c
    Core/Src/stm32h7rsxx_hal_msp.c
    Core/Src/stm32h7rsxx_it.c
    Core/Src/system_stm32h7rsxx.c
    Core/Src/extmem_manager.c
    ${CMAKE_SOURCE_DIR}/gcc/startup_stm32h7s7l8hxh_boot.s
)

# HAL Driver sources
file(GLOB HAL_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_cortex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_dma.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_dma_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_exti.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_flash.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_flash_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_pwr.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_pwr_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_rcc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_rcc_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_xspi.c
)

# ExtMem Manager sources
file(GLOB EXTMEM_SOURCES
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/*.c
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/boot/*.c
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/nor_sfdp/*.c
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/psram/*.c
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/sal/*.c
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/sdcard/*.c
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/user/*.c
)

add_executable(${PROJECT_NAME}
    ${BOOT_SOURCES}
    ${HAL_SOURCES}
    ${EXTMEM_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/boot
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/nor_sfdp
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_ExtMem_Manager/sal
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32H7S7xx
)

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/stm32h7s7xx_flash.ld)
target_link_options(${PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})

# Generate binary
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin"
)
