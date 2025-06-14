cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME pico-btpod) # <-- Name your project/executable here!
set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)
add_subdirectory(picomp3lib/src)

# Add your source files
add_executable(${NAME}
    main.cpp # <-- Add source files here!
    picow_bt_example_common.c
    a2dp_source_demo.c
    picomp3lib/interface/music_file.c
)

# Don't forget to link the libraries you need!
target_link_libraries(${NAME}
    pico_stdlib
    pico_multicore
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    pico_btstack_ble
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_threadsafe_background
    pico_btstack_sbc_encoder
    picomp3lib
)

target_compile_definitions(${NAME} PRIVATE
  CYW43_LWIP=0
  CYW43_PIO_CLOCK_DIV_INT=4
  CYW43_SPI_PROGRAM_NAME=spi_gap0_sample1
)

target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/config
    )

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Enable USB UART output only
pico_enable_stdio_uart(${NAME} 0)
pico_enable_stdio_usb(${NAME} 1)

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
