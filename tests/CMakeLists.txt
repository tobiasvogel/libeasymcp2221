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