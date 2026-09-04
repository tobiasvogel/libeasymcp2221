if (LIBEASYMCP2221_BUILD_STATIC)
    set(_LIBEASYMCP2221_HW_LINK_TARGET easymcp2221_static)
elseif (LIBEASYMCP2221_BUILD_SHARED)
    set(_LIBEASYMCP2221_HW_LINK_TARGET easymcp2221)
else()
    message(FATAL_ERROR "Hardware tests require a libeasymcp2221 library target")
endif()

function(add_libeasymcp2221_hardware_test target)
    add_executable(
        ${target}
        ${ARGN}
        hardware_test.c
    )

    target_include_directories(
        ${target}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(
        ${target}
        PRIVATE
            ${_LIBEASYMCP2221_HW_LINK_TARGET}
    )

    add_test(
        NAME ${target}
        COMMAND ${target}
    )

    set_tests_properties(
        ${target}
        PROPERTIES
            LABELS "hardware"
            SKIP_RETURN_CODE 77
    )
endfunction()

add_libeasymcp2221_hardware_test(test_hw_device test_hw_device.c)
add_libeasymcp2221_hardware_test(test_hw_gpio test_hw_gpio.c)
add_libeasymcp2221_hardware_test(test_hw_i2c_eeprom test_hw_i2c_eeprom.c)
add_libeasymcp2221_hardware_test(test_hw_analog test_hw_analog.c)
add_libeasymcp2221_hardware_test(test_hw_i2c_faults test_hw_i2c_faults.c)
add_libeasymcp2221_hardware_test(test_hw_i2c_chunks test_hw_i2c_chunks.c)
add_libeasymcp2221_hardware_test(test_hw_persistence test_hw_persistence.c)

unset(_LIBEASYMCP2221_HW_LINK_TARGET)
