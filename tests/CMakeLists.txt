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

add_libeasymcp2221_test(
    test_usb_config
    test_usb_config.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_usb.c
)

add_libeasymcp2221_test(
    test_error_codes
    test_error_codes.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_errors.c
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
