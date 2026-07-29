cmake_minimum_required(VERSION 3.10)
project(libeasymcp2221 C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(LIBEASYMCP2221_BUILD_STATIC "Build static library" ON)
option(LIBEASYMCP2221_BUILD_SHARED "Build shared library" ON)
option(LIBEASYMCP2221_BUILD_EXAMPLES "Build examples" ON)
option(LIBEASYMCP2221_INSTALL_UDEV_RULE "Install udev rule (non-Debian installs)" OFF)

if (NOT LIBEASYMCP2221_BUILD_SHARED AND NOT LIBEASYMCP2221_BUILD_STATIC)
    message(FATAL_ERROR "At least one of LIBEASYMCP2221_BUILD_SHARED or LIBEASYMCP2221_BUILD_STATIC must be ON")
endif()

set(LIBEASYMCP2221_INSTALL_TARGETS)
set(LIBEASYMCP2221_EXAMPLE_LINK_TARGET)


# libusb (in most Linux Distro's libusb-1.0)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${LIBUSB_INCLUDE_DIRS}
)

# Shared library
if (LIBEASYMCP2221_BUILD_SHARED)
    add_library(easymcp2221 SHARED
        src/mcp2221.c
        src/mcp2221_strings.c
        src/smbus.c
        src/i2c_slave.c
        src/mcp2221_gpio.c
        src/mcp2221_gpio_poll.c
        src/mcp2221_pin.c
        src/mcp2221_sram.c
        src/mcp2221_flash.c
        src/mcp2221_flash_settings.c
        src/mcp2221_flash_info.c
        src/mcp2221_analog.c
        src/exceptions.c
    )
    target_link_libraries(easymcp2221 ${LIBUSB_LIBRARIES})
    set_target_properties(easymcp2221 PROPERTIES
                    OUTPUT_NAME "easymcp2221"
                    SOVERSION 1
                    VERSION 1.1.1
    )
    list(APPEND LIBEASYMCP2221_INSTALL_TARGETS easymcp2221)
    set(LIBEASYMCP2221_EXAMPLE_LINK_TARGET easymcp2221)
endif()

# Static library
if (LIBEASYMCP2221_BUILD_STATIC)
    add_library(easymcp2221_static STATIC
        src/mcp2221.c
        src/mcp2221_strings.c
        src/smbus.c
        src/i2c_slave.c
        src/mcp2221_gpio.c
        src/mcp2221_gpio_poll.c
        src/mcp2221_pin.c
        src/mcp2221_sram.c
        src/mcp2221_flash.c
        src/mcp2221_flash_settings.c
        src/mcp2221_flash_info.c
        src/mcp2221_analog.c
        src/exceptions.c
    )
    target_link_libraries(easymcp2221_static ${LIBUSB_LIBRARIES})
    set_target_properties(easymcp2221_static PROPERTIES
                    OUTPUT_NAME "easymcp2221"
    )
    list(APPEND LIBEASYMCP2221_INSTALL_TARGETS easymcp2221_static)
    if (NOT LIBEASYMCP2221_EXAMPLE_LINK_TARGET)
        set(LIBEASYMCP2221_EXAMPLE_LINK_TARGET easymcp2221_static)
    endif()
endif()

# Install
install(TARGETS ${LIBEASYMCP2221_INSTALL_TARGETS}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libeasymcp2221
    FILES_MATCHING PATTERN "*.h"
    PATTERN "mcp2221_internal.h" EXCLUDE
)


# pkg-config
set(LIBEASYMCP2221_PKGCONFIG_LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(LIBEASYMCP2221_PKGCONFIG_INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/libeasymcp2221")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libeasymcp2221.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libeasymcp2221.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeasymcp2221.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(FILES libeasymcp2221.man.3
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man3
)

if (NOT DEFINED ENV{DEB_BUILD_OPTIONS})
    install(CODE "message(\"-- Running ldconfig\")")
    install(CODE "execute_process(COMMAND ldconfig)")
endif()

if (UNIX AND NOT APPLE AND LIBEASYMCP2221_INSTALL_UDEV_RULE)
    message(STATUS "Installing udev rule for MCP2221")

    install(FILES debian/udev.rules.d/99-mcp2221.rules
        DESTINATION lib/udev/rules.d
    )

    if (NOT DEFINED ENV{DEB_BUILD_OPTIONS})
        install(CODE "message(\"-- Reloading udev rules\")")
        install(CODE "execute_process(COMMAND udevadm control --reload-rules)")
        install(CODE "execute_process(COMMAND udevadm trigger)")
    endif()
endif()

# Examples
if (LIBEASYMCP2221_BUILD_EXAMPLES)
    add_subdirectory(examples)
    install(DIRECTORY examples/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
        FILES_MATCHING PATTERN "*.c"
    )
endif()
