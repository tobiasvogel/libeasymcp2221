cmake_minimum_required(VERSION 3.10)
project(libeasymcp2221 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(LIBEASYMCP2221_BUILD_STATIC "Build static library" ON)
option(LIBEASYMCP2221_BUILD_SHARED "Build shared library" ON)
option(LIBEASYMCP2221_BUILD_EXAMPLES "Build examples" ON)

# libusb (in most Linux Distro's libusb-1.0)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${LIBUSB_INCLUDE_DIRS}
)

# Shared library
if (LIBEASYMCP2221_BUILD_SHARED)
    add_library(libeasymcp2221_shared SHARED
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
    target_link_libraries(libeasymcp2221_shared ${LIBUSB_LIBRARIES})
    set_target_properties(libeasymcp2221_shared PROPERTIES OUTPUT_NAME "easymcp2221")    
endif()

# Static library
if (LIBEASYMCP2221_BUILD_STATIC)
    add_library(libeasymcp2221_static STATIC
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
    target_link_libraries(libeasymcp2221_static ${LIBUSB_LIBRARIES})
    set_target_properties(libeasymcp2221_static PROPERTIES OUTPUT_NAME "easymcp2221")    
endif()

# Install
install(TARGETS libeasymcp2221_shared libeasymcp2221_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY include/ DESTINATION include)


# pkg-config
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libeasymcp2221.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libeasymcp2221.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeasymcp2221.pc
    DESTINATION lib/pkgconfig)

# Examples
if (LIBEASYMCP2221_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()