function(add_libeasymcp2221_test target)
    add_executable(${target} ${ARGN})

    target_include_directories(${target}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    add_test(
        NAME ${target}
        COMMAND ${target}
    )
endfunction()

add_libeasymcp2221_test(
    test_analog_reference
    test_analog_reference.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
)

add_libeasymcp2221_test(
    test_analog_vdd
    test_analog_vdd.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
)

add_libeasymcp2221_test(
    test_adc_conversion
    test_adc_conversion.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
)

add_libeasymcp2221_test(
    test_dac_conversion
    test_dac_conversion.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
)

if (LIBEASYMCP2221_HAVE_NEXTAFTER)
    target_compile_definitions(
        test_dac_conversion
        PRIVATE LIBEASYMCP2221_HAVE_NEXTAFTER=1
    )
endif()

if (LIBEASYMCP2221_NEEDS_LIBM)
    target_link_libraries(test_dac_conversion PRIVATE m)
endif()

add_libeasymcp2221_test(
    test_usb_config
    test_usb_config.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_usb.c
)

add_libeasymcp2221_test(
    test_clock_config
    test_clock_config.c
)

add_libeasymcp2221_test(
    test_error_codes
    test_error_codes.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_errors.c
)

add_libeasymcp2221_test(
    test_smbus
    test_smbus.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_smbus.c
)

# mcp2221_send_cmd() lives in mcp2221.c together with the opaque device
# definition. The test includes that implementation directly so it can build
# a synthetic device and override libusb_interrupt_transfer().
add_executable(
    test_send_cmd_retry
    test_send_cmd_retry.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_strings.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_smbus.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_i2c_slave.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_gpio.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_gpio_poll.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_pin.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_sram.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_flash.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_flash_settings.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_flash_info.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_usb.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_usb.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_analog.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_errors.c
)

target_include_directories(test_send_cmd_retry
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${LIBUSB_INCLUDE_DIRS}
)

target_link_libraries(test_send_cmd_retry PRIVATE ${LIBUSB_LIBRARIES} Threads::Threads)
target_compile_definitions(test_send_cmd_retry PRIVATE LIBEASYMCP2221_BUILDING_LIBRARY)

add_test(NAME test_send_cmd_retry COMMAND test_send_cmd_retry)

# Endpoint discovery is implemented inside mcp2221.c. This test includes that
# implementation directly so it can provide synthetic libusb descriptors.
add_executable(
    test_usb_discovery
    test_usb_discovery.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_strings.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_smbus.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_i2c_slave.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_gpio.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_gpio_poll.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_pin.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_sram.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_flash.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_flash_settings.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_flash_info.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_usb.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_usb.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_analog.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_errors.c
)

target_include_directories(test_usb_discovery
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${LIBUSB_INCLUDE_DIRS}
)

target_link_libraries(test_usb_discovery PRIVATE ${LIBUSB_LIBRARIES} Threads::Threads)
target_compile_definitions(test_usb_discovery PRIVATE LIBEASYMCP2221_BUILDING_LIBRARY)

add_test(NAME test_usb_discovery COMMAND test_usb_discovery)

# Black-box protocol tests use a scripted libusb implementation instead of the
# real transport. Keep the production sources unchanged and link them directly
# into this executable together with fake_libusb.c.
set(_LIBEASYMCP2221_FAKE_TEST_SOURCES)
foreach(_source IN LISTS LIBEASYMCP2221_SOURCES)
    list(APPEND _LIBEASYMCP2221_FAKE_TEST_SOURCES
        "${PROJECT_SOURCE_DIR}/${_source}"
    )
endforeach()

add_executable(
    test_fake_libusb
    test_fake_libusb.c
    fake_libusb.c
    ${_LIBEASYMCP2221_FAKE_TEST_SOURCES}
)

target_include_directories(test_fake_libusb
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${LIBUSB_INCLUDE_DIRS}
)

target_link_libraries(test_fake_libusb PRIVATE Threads::Threads)
target_compile_definitions(test_fake_libusb PRIVATE LIBEASYMCP2221_STATIC)

if (LIBEASYMCP2221_HAVE_NEXTAFTER)
    target_compile_definitions(
        test_fake_libusb
        PRIVATE LIBEASYMCP2221_HAVE_NEXTAFTER=1
    )
endif()

if (LIBEASYMCP2221_NEEDS_LIBM)
    target_link_libraries(test_fake_libusb PRIVATE m)
endif()

add_test(NAME test_fake_libusb COMMAND test_fake_libusb)
unset(_LIBEASYMCP2221_FAKE_TEST_SOURCES)
