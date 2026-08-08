set(LIBEASYMCP2221_EXAMPLES
    test_i2c
    scan_i2c
    ssd1306_i2c
    gpio_poll_events
    set_pin_functions
    gpio_basic
    analog_demo
    flash_info_save
    usb_power
)

foreach(example_target IN LISTS LIBEASYMCP2221_EXAMPLES)
    add_executable(${example_target} ${example_target}.c)
    target_link_libraries(${example_target} ${LIBEASYMCP2221_EXAMPLE_LINK_TARGET} ${LIBUSB_LIBRARIES})
endforeach()
