add_executable(test_analog_reference
    test_analog_reference.c
    ${PROJECT_SOURCE_DIR}/src/mcp2221_internal_analog.c
)

target_include_directories(test_analog_reference
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

add_test(
    NAME analog_reference
    COMMAND test_analog_reference
)