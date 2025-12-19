add_executable(test_i2c test_i2c.c)
target_link_libraries(test_i2c easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(scan_i2c scan_i2c.c)
target_link_libraries(scan_i2c easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(ssd1306_i2c ssd1306_i2c.c)
target_link_libraries(ssd1306_i2c easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(gpio_poll_events gpio_poll_events.c)
target_link_libraries(gpio_poll_events easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(set_pin_functions set_pin_functions.c)
target_link_libraries(set_pin_functions easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(gpio_basic gpio_basic.c)
target_link_libraries(gpio_basic easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(analog_demo analog_demo.c)
target_link_libraries(analog_demo easymcp2221 ${LIBUSB_LIBRARIES})

add_executable(flash_info_save flash_info_save.c)
target_link_libraries(flash_info_save easymcp2221 ${LIBUSB_LIBRARIES})
