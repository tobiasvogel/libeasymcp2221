cmake_minimum_required(VERSION 3.10)
project(libeasymcp2221 VERSION 2.0.2 LANGUAGES C)

include(GNUInstallDirs)
include(CheckCSourceCompiles)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# MSVC has no C99 language mode. /std:c11 is the first standards mode that
# accepts the C99 source baseline used by this project.
if (MSVC)
    add_compile_options(/std:c11)
endif()

option(LIBEASYMCP2221_BUILD_STATIC "Build static library" ON)
option(LIBEASYMCP2221_BUILD_SHARED "Build shared library" ON)
option(LIBEASYMCP2221_BUILD_EXAMPLES "Build examples" ON)
option(LIBEASYMCP2221_INSTALL_UDEV_RULE "Install udev rule (non-Debian installs)" OFF)
option(LIBEASYMCP2221_BUILD_TESTS "Build unit tests" OFF)
option(LIBEASYMCP2221_BUILD_DOCS "Build API documentation with Doxygen" OFF)

if (NOT LIBEASYMCP2221_BUILD_SHARED AND NOT LIBEASYMCP2221_BUILD_STATIC)
    message(FATAL_ERROR "At least one of LIBEASYMCP2221_BUILD_SHARED or LIBEASYMCP2221_BUILD_STATIC must be ON")
endif()

set(LIBEASYMCP2221_INSTALL_TARGETS)
set(LIBEASYMCP2221_EXAMPLE_LINK_TARGET)


if (MSVC)
    # vcpkg's native Windows libusb port exposes headers and .lib files through
    # the CMake toolchain without requiring pkg-config.
    find_path(
        LIBUSB_INCLUDE_DIRS
        NAMES libusb.h
        PATH_SUFFIXES libusb-1.0
    )
    find_library(LIBUSB_LIBRARIES NAMES libusb-1.0)
    if (NOT LIBUSB_INCLUDE_DIRS OR NOT LIBUSB_LIBRARIES)
        message(FATAL_ERROR
            "libusb-1.0 was not found. For native MSVC builds, install "
            "libusb:x64-windows with vcpkg and configure CMake with the "
            "vcpkg toolchain file."
        )
    endif()
else()
    # libusb (in most Linux distributions and MinGW environments)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
endif()

# Threads::Threads is pthread-backed on POSIX/MinGW and a native/no-extra-lib
# abstraction on MSVC.
find_package(Threads REQUIRED)

set(LIBEASYMCP2221_NEXTAFTER_TEST_SOURCE "
    #include <math.h>
    int main(void) {
        double (*nextafter_fn)(double, double) = nextafter;
        volatile double x = 30.0;
        volatile double y = nextafter_fn(x, INFINITY);
        return y > x ? 0 : 1;
    }
")

check_c_source_compiles(
    "${LIBEASYMCP2221_NEXTAFTER_TEST_SOURCE}"
    LIBEASYMCP2221_HAVE_NEXTAFTER
)

set(LIBEASYMCP2221_NEEDS_LIBM OFF)
set(LIBEASYMCP2221_PKGCONFIG_PRIVATE_MATH_LIBS "")

if (NOT LIBEASYMCP2221_HAVE_NEXTAFTER)
    set(_LIBEASYMCP2221_SAVED_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES m)
    check_c_source_compiles(
        "${LIBEASYMCP2221_NEXTAFTER_TEST_SOURCE}"
        LIBEASYMCP2221_HAVE_NEXTAFTER_LIBM
    )
    set(CMAKE_REQUIRED_LIBRARIES "${_LIBEASYMCP2221_SAVED_REQUIRED_LIBRARIES}")
    unset(_LIBEASYMCP2221_SAVED_REQUIRED_LIBRARIES)

    if (LIBEASYMCP2221_HAVE_NEXTAFTER_LIBM)
        set(LIBEASYMCP2221_HAVE_NEXTAFTER ON)
        set(LIBEASYMCP2221_NEEDS_LIBM ON)
        set(LIBEASYMCP2221_PKGCONFIG_PRIVATE_MATH_LIBS " -lm")
    endif()
endif()

if (LIBEASYMCP2221_HAVE_NEXTAFTER)
    if (LIBEASYMCP2221_NEEDS_LIBM)
        message(STATUS "DAC floating-point stabilization: nextafter with libm")
    else()
        message(STATUS "DAC floating-point stabilization: nextafter available")
    endif()
else()
    message(STATUS "DAC floating-point stabilization: DBL_EPSILON fallback")
endif()

set(LIBEASYMCP2221_SOURCES
    src/mcp2221.c
    src/mcp2221_strings.c
    src/mcp2221_smbus.c
    src/mcp2221_i2c_slave.c
    src/mcp2221_gpio.c
    src/mcp2221_gpio_poll.c
    src/mcp2221_pin.c
    src/mcp2221_sram.c
    src/mcp2221_flash.c
    src/mcp2221_flash_settings.c
    src/mcp2221_flash_info.c
    src/mcp2221_usb.c
    src/mcp2221_internal_usb.c
    src/mcp2221_analog.c
    src/mcp2221_internal_analog.c
    src/mcp2221_errors.c
)

set(LIBEASYMCP2221_SHARED_OUTPUT_NAME "easymcp2221")
set(LIBEASYMCP2221_STATIC_OUTPUT_NAME "easymcp2221")
if (MSVC)
    # Keep the project/library name in the DLL while avoiding the MSVC .lib
    # collision between the DLL import library and the static archive.
    set(LIBEASYMCP2221_SHARED_OUTPUT_NAME "libeasymcp2221")
    set(LIBEASYMCP2221_STATIC_OUTPUT_NAME "libeasymcp2221_static")
endif()

function(libeasymcp2221_configure_target target)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${LIBUSB_INCLUDE_DIRS}
    )
    target_link_libraries(${target} PRIVATE ${LIBUSB_LIBRARIES} Threads::Threads)
    if (LIBEASYMCP2221_NEEDS_LIBM)
        target_link_libraries(${target} PRIVATE m)
    endif()
    if (LIBEASYMCP2221_HAVE_NEXTAFTER)
        target_compile_definitions(
            ${target}
            PRIVATE LIBEASYMCP2221_HAVE_NEXTAFTER=1
        )
    endif()
endfunction()

# Shared library
if (LIBEASYMCP2221_BUILD_SHARED)
    add_library(easymcp2221 SHARED ${LIBEASYMCP2221_SOURCES})
    libeasymcp2221_configure_target(easymcp2221)
    target_compile_definitions(easymcp2221
        PRIVATE LIBEASYMCP2221_BUILDING_LIBRARY
    )
    set_target_properties(easymcp2221 PROPERTIES
        C_VISIBILITY_PRESET hidden
        OUTPUT_NAME "${LIBEASYMCP2221_SHARED_OUTPUT_NAME}"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
    )
    list(APPEND LIBEASYMCP2221_INSTALL_TARGETS easymcp2221)
    set(LIBEASYMCP2221_EXAMPLE_LINK_TARGET easymcp2221)
endif()

# Static library
if (LIBEASYMCP2221_BUILD_STATIC)
    add_library(easymcp2221_static STATIC ${LIBEASYMCP2221_SOURCES})
    libeasymcp2221_configure_target(easymcp2221_static)
    target_compile_definitions(easymcp2221_static PUBLIC LIBEASYMCP2221_STATIC)
    set_target_properties(easymcp2221_static PROPERTIES
        OUTPUT_NAME "${LIBEASYMCP2221_STATIC_OUTPUT_NAME}"
    )
    list(APPEND LIBEASYMCP2221_INSTALL_TARGETS easymcp2221_static)
    if (NOT LIBEASYMCP2221_EXAMPLE_LINK_TARGET)
        set(LIBEASYMCP2221_EXAMPLE_LINK_TARGET easymcp2221_static)
    endif()
endif()

# Install
install(TARGETS ${LIBEASYMCP2221_INSTALL_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libeasymcp2221
    FILES_MATCHING PATTERN "*.h"
    PATTERN "mcp2221_internal*.h" EXCLUDE
)


# pkg-config metadata is the supported consumer path for POSIX and MinGW.
if (NOT MSVC)
    set(LIBEASYMCP2221_PKGCONFIG_LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
    set(LIBEASYMCP2221_PKGCONFIG_INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/libeasymcp2221.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/libeasymcp2221.pc
        @ONLY
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeasymcp2221.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()

install(FILES libeasymcp2221.man.3
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man3
)

if (UNIX AND NOT APPLE AND LIBEASYMCP2221_INSTALL_UDEV_RULE)
    message(STATUS "Installing udev rule for MCP2221")

    install(FILES debian/libeasymcp2221-2.udev
        DESTINATION lib/udev/rules.d
        RENAME 60-mcp2221.rules
    )
endif()

# Examples
if (LIBEASYMCP2221_BUILD_EXAMPLES)
    add_subdirectory(examples)
    install(DIRECTORY examples/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
        FILES_MATCHING PATTERN "*.c"
    )
endif()

# Unit-Tests
if (LIBEASYMCP2221_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if (LIBEASYMCP2221_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    if (NOT DOXYGEN_DOT_FOUND)
        message(FATAL_ERROR
            "Graphviz 'dot' is required to build the API documentation"
        )
    endif()

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating libeasymcp2221 API documentation"
        VERBATIM
    )
endif()