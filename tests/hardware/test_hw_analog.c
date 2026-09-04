#include "hardware_test.h"

#include <stdio.h>

#define ANALOG_SETTLE_MS 10u
#define ANALOG_RAW_TOLERANCE 64u

typedef struct {
    const char *ref;
    uint8_t dac_code;
} analog_case_t;

static int configure_analog_fixture(mcp2221_t *dev)
{
    const mcp2221_pin_functions_t cfg = {
        {
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_ALT0,
            MCP2221_PIN_FUNC_ALT1
        },
        {0, 0, 0, 0}
    };
    mcp2221_error_code_t rc;

    /*
     * GP2 ALT0 is ADC2 and GP3 ALT1 is DAC2. Configure the pin mux before
     * touching ADC/DAC references because SRAM GPIO updates can reinitialize
     * the voltage-reference modules.
     */
    rc = mcp2221_pin_set_functions(dev, &cfg);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("configuring ADC/DAC fixture pins", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static int run_analog_case(mcp2221_t *dev, const analog_case_t *test_case)
{
    uint16_t adc[3];
    unsigned int expected;
    unsigned int actual;
    unsigned int error;
    mcp2221_error_code_t rc;

    rc = mcp2221_adc_config(dev, test_case->ref);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("configuring ADC reference", rc);
        return HW_TEST_FAILED;
    }

    rc = mcp2221_dac_config(dev, test_case->ref);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("configuring DAC reference", rc);
        return HW_TEST_FAILED;
    }

    rc = mcp2221_dac_write_raw(dev, test_case->dac_code);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("writing DAC code", rc);
        return HW_TEST_FAILED;
    }

    hw_test_sleep_ms(ANALOG_SETTLE_MS);

    rc = mcp2221_adc_read_raw(dev, adc);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("reading ADC", rc);
        return HW_TEST_FAILED;
    }

    /*
     * With identical ADC and DAC references:
     *
     *   ADC = (DAC_code / 32) * 1024 = DAC_code * 32
     *
     * ADC channel index 1 is GP2, which is connected to the GP3 DAC output by
     * the EasyMCP2221 reference test fixture.
     */
    expected = (unsigned int)test_case->dac_code * 32u;
    actual = (unsigned int)adc[1];
    error = (actual > expected) ? (actual - expected) : (expected - actual);

    if (error > ANALOG_RAW_TOLERANCE) {
        fprintf(stderr,
                "ADC/DAC loopback mismatch: ref=%s DAC=%u expected=%u "
                "got=%u (error=%u, tolerance=%u)\n",
                test_case->ref,
                (unsigned int)test_case->dac_code,
                expected,
                actual,
                error,
                (unsigned int)ANALOG_RAW_TOLERANCE);
        return HW_TEST_FAILED;
    }

    printf("ADC/DAC case: ref=%s DAC=%u expected=%u got=%u\n",
           test_case->ref,
           (unsigned int)test_case->dac_code,
           expected,
           actual);

    return HW_TEST_OK;
}

int main(void)
{
    static const analog_case_t cases[] = {
        {"1.024V", 10u},
        {"1.024V", 16u},
        {"1.024V", 24u},
        {"2.048V", 10u},
        {"2.048V", 16u},
        {"2.048V", 24u}
    };
    hw_test_config_t cfg;
    mcp2221_t *dev;
    mcp2221_error_code_t rc;
    int result;
    int cleanup_result;
    size_t i;

    if (hw_test_load_config(&cfg) != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    result = hw_test_open(&cfg, &dev);
    if (result != HW_TEST_OK) {
        return result;
    }

    result = configure_analog_fixture(dev);
    if (result == HW_TEST_OK) {
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
            result = run_analog_case(dev, &cases[i]);
            if (result != HW_TEST_OK) {
                break;
            }
        }
    }

    rc = mcp2221_dac_write_raw(dev, 0u);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("clearing DAC output", rc);
        result = HW_TEST_FAILED;
    }

    cleanup_result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (cleanup_result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("PASS: GP3 DAC -> GP2 ADC loopback across fixed references\n");
    return HW_TEST_OK;
}
