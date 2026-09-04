#include "hardware_test.h"

#include <stdio.h>
#include <string.h>

#define EEPROM_MEMORY_ADDRESS 0x0000u
#define EEPROM_WRITE_CYCLE_MS 10u

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
        hw_test_print_error("configuring I2C fixture GPIOs", rc);
        return HW_TEST_FAILED;
    }

    /*
     * The fixture can leave the MCP2221 I2C engine dirty while GP0/GP1 hold
     * SCL/SDA low. Release the engine only after those lines have been driven
     * high, then apply the requested bus speed.
     */
    rc = mcp2221_i2c_release(dev);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("releasing I2C bus", rc);
        return HW_TEST_FAILED;
    }

    rc = mcp2221_i2c_set_speed(dev, 100000);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("setting I2C speed", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static void eeprom_address_bytes(uint8_t address_buf[2])
{
    address_buf[0] = (uint8_t)((EEPROM_MEMORY_ADDRESS >> 8) & 0xffu);
    address_buf[1] = (uint8_t)(EEPROM_MEMORY_ADDRESS & 0xffu);
}

static mcp2221_error_code_t eeprom_read(mcp2221_t *dev, uint8_t addr,
                                        uint8_t *data, size_t len)
{
    uint8_t address_buf[2];
    mcp2221_error_code_t rc;

    eeprom_address_bytes(address_buf);

    rc = mcp2221_i2c_write_simple(
        dev, addr, address_buf, sizeof(address_buf), MCP2221_I2C_KIND_NO_STOP);
    if (rc != MCP2221_ERR_OK) {
        return rc;
    }

    return mcp2221_i2c_read_simple(
        dev, addr, data, len, MCP2221_I2C_KIND_REPEATED_START);
}

static mcp2221_error_code_t eeprom_write(mcp2221_t *dev, uint8_t addr,
                                         const uint8_t *data, size_t len)
{
    uint8_t write_buf[2 + 16];
    mcp2221_error_code_t rc;

    if (len > sizeof(write_buf) - 2u) {
        return MCP2221_ERR_INVALID;
    }

    eeprom_address_bytes(write_buf);
    memcpy(&write_buf[2], data, len);

    rc = mcp2221_i2c_write_simple(
        dev, addr, write_buf, len + 2u, MCP2221_I2C_KIND_NORMAL);
    if (rc == MCP2221_ERR_OK) {
        hw_test_sleep_ms(EEPROM_WRITE_CYCLE_MS);
    }

    return rc;
}

static int eeprom_write_readback(mcp2221_t *dev, uint8_t addr)
{
    static const uint8_t expected[] = {
        0x4c, 0x69, 0x62, 0x45, 0x61, 0x73, 0x79, 0x4d,
        0x43, 0x50, 0x32, 0x32, 0x32, 0x31, 0x02, 0x00
    };
    uint8_t original[sizeof(expected)];
    uint8_t actual[sizeof(expected)];
    uint8_t restored[sizeof(expected)];
    mcp2221_error_code_t rc;
    int result = HW_TEST_OK;
    int original_saved = 0;

    rc = eeprom_read(dev, addr, original, sizeof(original));
    if (rc == MCP2221_ERR_NOT_ACK) {
        printf("SKIP: no EEPROM acknowledged at I2C address 0x%02x\n",
               (unsigned int)addr);
        return HW_TEST_SKIPPED;
    }
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("saving original EEPROM contents", rc);
        return HW_TEST_FAILED;
    }
    original_saved = 1;

    rc = eeprom_write(dev, addr, expected, sizeof(expected));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("writing EEPROM test pattern", rc);
        result = HW_TEST_FAILED;
        goto restore;
    }

    rc = eeprom_read(dev, addr, actual, sizeof(actual));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("reading EEPROM test pattern", rc);
        result = HW_TEST_FAILED;
        goto restore;
    }

    if (memcmp(expected, actual, sizeof(expected)) != 0) {
        size_t i;

        fprintf(stderr, "EEPROM readback mismatch:\n");
        for (i = 0; i < sizeof(expected); ++i) {
            if (expected[i] != actual[i]) {
                fprintf(stderr,
                        "  offset %zu: expected 0x%02x, got 0x%02x\n",
                        i, (unsigned int)expected[i], (unsigned int)actual[i]);
            }
        }
        result = HW_TEST_FAILED;
    }

restore:
    if (original_saved) {
        rc = eeprom_write(dev, addr, original, sizeof(original));
        if (rc != MCP2221_ERR_OK) {
            hw_test_print_error("restoring original EEPROM contents", rc);
            return HW_TEST_FAILED;
        }

        rc = eeprom_read(dev, addr, restored, sizeof(restored));
        if (rc != MCP2221_ERR_OK) {
            hw_test_print_error("verifying restored EEPROM contents", rc);
            return HW_TEST_FAILED;
        }

        if (memcmp(original, restored, sizeof(original)) != 0) {
            fprintf(stderr, "restored EEPROM contents do not match original data\n");
            return HW_TEST_FAILED;
        }
    }

    return result;
}

int main(void)
{
    hw_test_config_t cfg;
    mcp2221_t *dev;
    int result;
    int cleanup_result;

    if (hw_test_load_config(&cfg) != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    result = hw_test_open(&cfg, &dev);
    if (result != HW_TEST_OK) {
        return result;
    }

    result = configure_i2c_fixture(dev);
    if (result == HW_TEST_OK) {
        result = eeprom_write_readback(dev, cfg.eeprom_addr);
    }

    cleanup_result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (cleanup_result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("PASS: EEPROM 0x%02x deterministic write/readback at 100 kHz "
           "with original contents restored\n",
           (unsigned int)cfg.eeprom_addr);
    return HW_TEST_OK;
}
