add_executable(test_i2c test_i2c.c)
target_link_libraries(test_i2c libeasymcp2221_shared ${LIBUSB_LIBRARIES})