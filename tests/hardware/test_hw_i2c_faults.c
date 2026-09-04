#include "hardware_test.h"

#include <stdio.h>

#define EEPROM_MEMORY_ADDRESS 0x0000u

typedef enum {
    FAULT_SCL_LOW,
    FAULT_SDA_LOW
} fault_line_t;

typedef enum {
    OP_READ,
    OP_WRITE
} fault_operation_t;

typedef struct {
    const char *name;
    fault_line_t line;
    fault_operation_t operation;
    mcp2221_error_code_t expected_error;
} fault_case_t;

static int configure_i2c_fixture(mcp2221_t *dev)
{
    const mcp2221_pin_functions_t cfg = {
        {
            MCP2221_PIN_FUNC_GPIO_OUT,
            MCP2221_PIN_FUNC_GPIO_OUT,
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN
        },
        {1, 1, 0, 0}
    };
    mcp2221_error_code_t rc;

    rc = mcp2221_pin_set_functions(dev, &cfg);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("configuring I2C fault fixture GPIOs", rc);
        return HW_TEST_FAILED;
    }

    rc = mcp2221_i2c_release(dev);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("releasing I2C bus during setup", rc);
        return HW_TEST_FAILED;
    }

    rc = mcp2221_i2c_set_speed(dev, 100000);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("setting I2C speed during setup", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static int set_fault_line(mcp2221_t *dev, fault_line_t line, int level)
{
    mcp2221_gpio_write_t wr = {
        MCP2221_GPIO_KEEP,
        MCP2221_GPIO_KEEP,
        MCP2221_GPIO_KEEP,
        MCP2221_GPIO_KEEP
    };
    mcp2221_error_code_t rc;

    if (line == FAULT_SCL_LOW) {
        wr.gp0 = level;
    } else {
        wr.gp1 = level;
    }

    rc = mcp2221_gpio_write(dev, &wr);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error(level ? "releasing fault GPIO" : "asserting fault GPIO", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static int probe_eeprom(mcp2221_t *dev, uint8_t addr)
{
    uint8_t value;
    mcp2221_error_code_t rc;

    rc = mcp2221_i2c_read_simple(
        dev, addr, &value, 1u, MCP2221_I2C_KIND_NORMAL);
    if (rc == MCP2221_ERR_NOT_ACK) {
        printf("SKIP: no EEPROM acknowledged at I2C address 0x%02x\n",
               (unsigned int)addr);
        return HW_TEST_SKIPPED;
    }
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("probing EEPROM after recovery", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static mcp2221_error_code_t perform_fault_operation(
    mcp2221_t *dev, uint8_t addr, fault_operation_t operation)
{
    uint8_t value;
    const uint8_t address_buf[2] = {
        (uint8_t)((EEPROM_MEMORY_ADDRESS >> 8) & 0xffu),
        (uint8_t)(EEPROM_MEMORY_ADDRESS & 0xffu)
    };

    if (operation == OP_READ) {
        return mcp2221_i2c_read_simple(
            dev, addr, &value, 1u, MCP2221_I2C_KIND_NORMAL);
    }

    /*
     * Sending only the EEPROM memory-address bytes changes its internal
     * address pointer but does not write EEPROM data.
     */
    return mcp2221_i2c_write_simple(
        dev, addr, address_buf, sizeof(address_buf), MCP2221_I2C_KIND_NORMAL);
}

static int recover_bus(mcp2221_t *dev, fault_line_t line, uint8_t addr)
{
    mcp2221_error_code_t rc;
    int result;

    result = set_fault_line(dev, line, 1);
    if (result != HW_TEST_OK) {
        return result;
    }

    rc = mcp2221_i2c_release(dev);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("recovering I2C bus", rc);
        return HW_TEST_FAILED;
    }

    return probe_eeprom(dev, addr);
}

static int run_fault_case(mcp2221_t *dev, uint8_t addr,
                          const fault_case_t *test_case)
{
    mcp2221_error_code_t rc;
    int result;

    result = set_fault_line(dev, test_case->line, 0);
    if (result != HW_TEST_OK) {
        return result;
    }

    rc = perform_fault_operation(dev, addr, test_case->operation);
    if (rc != test_case->expected_error) {
        fprintf(stderr,
                "%s: expected %s (%d), got %s (%d)\n",
                test_case->name,
                mcp2221_error_code_to_string(test_case->expected_error),
                (int)test_case->expected_error,
                mcp2221_error_code_to_string(rc),
                (int)rc);

        /*
         * Always release the injected fault before returning, even when the
         * observed library error was unexpected.
         */
        (void)set_fault_line(dev, test_case->line, 1);
        (void)mcp2221_i2c_release(dev);
        return HW_TEST_FAILED;
    }

    result = recover_bus(dev, test_case->line, addr);
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("I2C fault case: %s -> %s, recovery OK\n",
           test_case->name,
           mcp2221_error_code_to_string(test_case->expected_error));
    return HW_TEST_OK;
}

int main(void)
{
    static const fault_case_t cases[] = {
        {"SCL low during read", FAULT_SCL_LOW, OP_READ, MCP2221_ERR_LOW_SCL},
        {"SCL low during write", FAULT_SCL_LOW, OP_WRITE, MCP2221_ERR_LOW_SCL},
        {"SDA low during read", FAULT_SDA_LOW, OP_READ, MCP2221_ERR_LOW_SDA},
        {"SDA low during write", FAULT_SDA_LOW, OP_WRITE, MCP2221_ERR_LOW_SDA}
    };
    hw_test_config_t cfg;
    mcp2221_t *dev;
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

    result = configure_i2c_fixture(dev);
    if (result == HW_TEST_OK) {
        result = probe_eeprom(dev, cfg.eeprom_addr);
    }

    if (result == HW_TEST_OK) {
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
            result = run_fault_case(dev, cfg.eeprom_addr, &cases[i]);
            if (result != HW_TEST_OK) {
                break;
            }
        }
    }

    /*
     * Ensure both fixture pull-up GPIOs are released high before switching
     * everything to the shared non-driving safe state.
     */
    {
        const mcp2221_gpio_write_t release = {
            1, 1, MCP2221_GPIO_KEEP, MCP2221_GPIO_KEEP
        };
        (void)mcp2221_gpio_write(dev, &release);
        (void)mcp2221_i2c_release(dev);
    }

    cleanup_result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (cleanup_result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("PASS: I2C SCL/SDA fault detection and recovery\n");
    return HW_TEST_OK;
}
