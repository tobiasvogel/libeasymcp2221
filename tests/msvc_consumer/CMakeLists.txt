cmake_minimum_required(VERSION 3.10)
project(libeasymcp2221_msvc_consumer LANGUAGES C)

if (NOT MSVC)
    message(FATAL_ERROR "This consumer smoke test requires native MSVC")
endif()

if (NOT LIBEASYMCP2221_INSTALL_DIR)
    message(FATAL_ERROR "LIBEASYMCP2221_INSTALL_DIR is required")
endif()

set(_LIBEASYMCP2221_INCLUDE_DIR
    "${LIBEASYMCP2221_INSTALL_DIR}/include"
)
set(_LIBEASYMCP2221_SHARED_IMPORT
    "${LIBEASYMCP2221_INSTALL_DIR}/lib/libeasymcp2221.lib"
)
set(_LIBEASYMCP2221_STATIC
    "${LIBEASYMCP2221_INSTALL_DIR}/lib/libeasymcp2221_static.lib"
)

foreach(_required
    "${_LIBEASYMCP2221_SHARED_IMPORT}"
    "${_LIBEASYMCP2221_STATIC}"
)
    if (NOT EXISTS "${_required}")
        message(FATAL_ERROR "Missing installed library: ${_required}")
    endif()
endforeach()

find_library(LIBUSB_LIBRARY NAMES libusb-1.0)
if (NOT LIBUSB_LIBRARY)
    message(FATAL_ERROR "libusb-1.0 was not found")
endif()

add_executable(msvc_shared_consumer consumer.c)
target_include_directories(
    msvc_shared_consumer
    PRIVATE "${_LIBEASYMCP2221_INCLUDE_DIR}"
)
target_link_libraries(
    msvc_shared_consumer
    PRIVATE "${_LIBEASYMCP2221_SHARED_IMPORT}"
)

add_executable(msvc_static_consumer consumer.c)
target_include_directories(
    msvc_static_consumer
    PRIVATE "${_LIBEASYMCP2221_INCLUDE_DIR}"
)
target_compile_definitions(
    msvc_static_consumer
    PRIVATE LIBEASYMCP2221_STATIC
)
target_link_libraries(
    msvc_static_consumer
    PRIVATE
        "${_LIBEASYMCP2221_STATIC}"
        "${LIBUSB_LIBRARY}"
)
