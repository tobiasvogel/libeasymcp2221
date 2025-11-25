cmake_minimum_required(VERSION 3.10)
project(libeasymcp2221)

set(CMAKE_C_STANDARD 99)

# libusb suchen (unter Linux meist libusb-1.0)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${LIBUSB_INCLUDE_DIRS}
)

add_library(libeasymcp2221
    src/mcp2221.c
    src/smbus.c
    src/i2c_slave.c
    src/mcp2221_gpio.c
    src/mcp2221_gpio_poll.c
    src/mcp2221_pin.c
    src/mcp2221_sram.c
    src/mcp2221_flash.c
    src/mcp2221_flash_settings.c
    src/mcp2221_analog.c
)

target_link_libraries(libeasymcp2221
    ${LIBUSB_LIBRARIES}
)

# Beispielprogramm
add_executable(test_i2c examples/test_i2c.c)
target_link_libraries(test_i2c libeasymcp2221)
