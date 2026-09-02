#include "hardware_test.h"

#include <stdio.h>

static int set_loopback_state(mcp2221_t *dev, int gp2_value)
{
    mcp2221_pin_functions_t cfg = {
        {
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_OUT,
            MCP2221_PIN_FUNC_GPIO_IN
        },
        {0, 0, 0, 0}
    };
    mcp2221_error_code_t rc;

    cfg.out[2] = gp2_value;

    rc = mcp2221_pin_set_functions(dev, &cfg);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("configuring GP2/GP3 loopback", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static int expect_gp3(mcp2221_t *dev, int expected)
{
    int state[4];
    uint8_t valid_mask;
    mcp2221_error_code_t rc;

    rc = mcp2221_gpio_read_mask(dev, state, &valid_mask);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("mcp2221_gpio_read_mask", rc);
        return HW_TEST_FAILED;
    }

    if ((valid_mask & (1u << 3)) == 0u) {
        fprintf(stderr, "GP3 is not reported as GPIO\n");
        return HW_TEST_FAILED;
    }

    if (state[3] != expected) {
        fprintf(stderr, "GP3 loopback mismatch: expected %d, got %d\n",
                expected, state[3]);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

int main(void)
{
    hw_test_config_t cfg;
    mcp2221_t *dev;
    mcp2221_gpio_write_t write;
    mcp2221_error_code_t rc;
    int result;
    int cleanup_result;

    if (hw_test_load_config(&cfg) != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    result = hw_test_open(&cfg, &dev);
    if (result != HW_TEST_OK) {
        return result;
    }

    result = set_loopback_state(dev, 0);
    if (result == HW_TEST_OK) {
        hw_test_sleep_ms(10);
        result = expect_gp3(dev, 0);
    }

    if (result == HW_TEST_OK) {
        write.gp0 = MCP2221_GPIO_KEEP;
        write.gp1 = MCP2221_GPIO_KEEP;
        write.gp2 = 1;
        write.gp3 = MCP2221_GPIO_KEEP;

        rc = mcp2221_gpio_write(dev, &write);
        if (rc != MCP2221_ERR_OK) {
            hw_test_print_error("driving GP2 high", rc);
            result = HW_TEST_FAILED;
        }
    }

    if (result == HW_TEST_OK) {
        hw_test_sleep_ms(10);
        result = expect_gp3(dev, 1);
    }

    if (result == HW_TEST_OK) {
        write.gp2 = 0;
        rc = mcp2221_gpio_write(dev, &write);
        if (rc != MCP2221_ERR_OK) {
            hw_test_print_error("driving GP2 low", rc);
            result = HW_TEST_FAILED;
        }
    }

    if (result == HW_TEST_OK) {
        hw_test_sleep_ms(10);
        result = expect_gp3(dev, 0);
    }

    cleanup_result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (cleanup_result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("PASS: GP2 output is observed on GP3 input (LOW/HIGH/LOW)\n");
    return HW_TEST_OK;
}
