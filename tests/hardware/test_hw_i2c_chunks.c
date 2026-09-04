#include "hardware_test.h"

#include <stdio.h>
#include <string.h>

#define EEPROM_MEMORY_ADDRESS 0x0000u
#define EEPROM_PAGE_SIZE 64u
#define EEPROM_WRITE_CYCLE_MS 10u
#define MISSING_I2C_ADDRESS 0x51u

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
        hw_test_print_error("configuring I2C chunk fixture GPIOs", rc);
        return HW_TEST_FAILED;
    }

    HW_TEST_RETRY_TIMEOUT_ONCE(rc, mcp2221_i2c_release(dev));
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
    uint8_t write_buf[2 + EEPROM_PAGE_SIZE];
    mcp2221_error_code_t rc;

    if (len > EEPROM_PAGE_SIZE) {
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

static int expect_not_ack(mcp2221_t *dev, uint8_t addr, size_t len, int is_read)
{
    uint8_t data[65];
    mcp2221_error_code_t rc;
    size_t i;

    for (i = 0; i < sizeof(data); ++i) {
        data[i] = (uint8_t)(0xa0u + (uint8_t)i);
    }

    if (is_read) {
        rc = mcp2221_i2c_read_simple(
            dev, addr, data, len, MCP2221_I2C_KIND_NORMAL);
    } else {
        rc = mcp2221_i2c_write_simple(
            dev, addr, data, len, MCP2221_I2C_KIND_NORMAL);
    }

    if (rc != MCP2221_ERR_NOT_ACK) {
        fprintf(stderr,
                "missing-address %s len=%zu: expected %s (%d), got %s (%d)\n",
                is_read ? "read" : "write",
                len,
                mcp2221_error_code_to_string(MCP2221_ERR_NOT_ACK),
                (int)MCP2221_ERR_NOT_ACK,
                mcp2221_error_code_to_string(rc),
                (int)rc);
        return HW_TEST_FAILED;
    }

    /*
     * A NACK can leave the MCP2221 I2C engine in a dirty state. The Python
     * reference suite isolates each NACK case with fresh setUp()/tearDown();
     * explicitly release the engine here to give the combined C test the same
     * isolation before the next operation.
     */
    HW_TEST_RETRY_TIMEOUT_ONCE(rc, mcp2221_i2c_release(dev));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("releasing I2C bus after expected NACK", rc);
        return HW_TEST_FAILED;
    }

    printf("I2C NACK case: %s len=%zu at 0x%02x -> NotAckError, recovery OK\n",
           is_read ? "read" : "write",
           len,
           (unsigned int)addr);
    return HW_TEST_OK;
}

static void fill_pattern(uint8_t *data, size_t len, size_t test_len)
{
    size_t i;

    for (i = 0; i < len; ++i) {
        data[i] = (uint8_t)((0x31u + (unsigned int)i * 17u +
                             (unsigned int)test_len * 3u) & 0xffu);
    }
}

static int test_chunk_lengths(mcp2221_t *dev, uint8_t addr)
{
    uint8_t original[EEPROM_PAGE_SIZE];
    uint8_t pattern[EEPROM_PAGE_SIZE];
    uint8_t actual[EEPROM_PAGE_SIZE];
    uint8_t restored[EEPROM_PAGE_SIZE];
    mcp2221_error_code_t rc;
    int result = HW_TEST_OK;
    int original_saved = 0;
    size_t len;

    HW_TEST_RETRY_TIMEOUT_ONCE(
        rc, eeprom_read(dev, addr, original, sizeof(original)));
    if (rc == MCP2221_ERR_NOT_ACK) {
        printf("SKIP: no EEPROM acknowledged at I2C address 0x%02x\n",
               (unsigned int)addr);
        return HW_TEST_SKIPPED;
    }
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("saving EEPROM page before chunk tests", rc);
        return HW_TEST_FAILED;
    }
    original_saved = 1;

    for (len = 55u; len <= EEPROM_PAGE_SIZE; ++len) {
        fill_pattern(pattern, len, len);

        rc = eeprom_write(dev, addr, pattern, len);
        if (rc != MCP2221_ERR_OK) {
            fprintf(stderr, "EEPROM chunk write len=%zu failed: ", len);
            hw_test_print_error("mcp2221_i2c_write_simple", rc);
            result = HW_TEST_FAILED;
            break;
        }

        memset(actual, 0, sizeof(actual));
        HW_TEST_RETRY_TIMEOUT_ONCE(
            rc, eeprom_read(dev, addr, actual, len));
        if (rc != MCP2221_ERR_OK) {
            fprintf(stderr, "EEPROM chunk read len=%zu failed: ", len);
            hw_test_print_error("mcp2221_i2c_read_simple", rc);
            result = HW_TEST_FAILED;
            break;
        }

        if (memcmp(pattern, actual, len) != 0) {
            size_t i;

            fprintf(stderr, "EEPROM chunk mismatch at len=%zu\n", len);
            for (i = 0; i < len; ++i) {
                if (pattern[i] != actual[i]) {
                    fprintf(stderr,
                            "  offset %zu: expected 0x%02x, got 0x%02x\n",
                            i,
                            (unsigned int)pattern[i],
                            (unsigned int)actual[i]);
                    break;
                }
            }
            result = HW_TEST_FAILED;
            break;
        }

        printf("I2C chunk case: EEPROM write/read len=%zu OK\n", len);
    }

    if (original_saved) {
        rc = eeprom_write(dev, addr, original, sizeof(original));
        if (rc != MCP2221_ERR_OK) {
            hw_test_print_error("restoring EEPROM page after chunk tests", rc);
            return HW_TEST_FAILED;
        }

        HW_TEST_RETRY_TIMEOUT_ONCE(
            rc, eeprom_read(dev, addr, restored, sizeof(restored)));
        if (rc != MCP2221_ERR_OK) {
            hw_test_print_error("verifying restored EEPROM page", rc);
            return HW_TEST_FAILED;
        }

        if (memcmp(original, restored, sizeof(original)) != 0) {
            fprintf(stderr, "restored EEPROM page does not match original data\n");
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

    if (result == HW_TEST_OK && cfg.eeprom_addr == MISSING_I2C_ADDRESS) {
        fprintf(stderr,
                "configured EEPROM address conflicts with missing-address "
                "test address 0x%02x\n",
                (unsigned int)MISSING_I2C_ADDRESS);
        result = HW_TEST_FAILED;
    }

    if (result == HW_TEST_OK) {
        result = expect_not_ack(dev, MISSING_I2C_ADDRESS, 1u, 1);
    }
    if (result == HW_TEST_OK) {
        result = expect_not_ack(dev, MISSING_I2C_ADDRESS, 40u, 1);
    }
    if (result == HW_TEST_OK) {
        result = expect_not_ack(dev, MISSING_I2C_ADDRESS, 65u, 1);
    }
    if (result == HW_TEST_OK) {
        result = expect_not_ack(dev, MISSING_I2C_ADDRESS, 1u, 0);
    }
    if (result == HW_TEST_OK) {
        result = expect_not_ack(dev, MISSING_I2C_ADDRESS, 40u, 0);
    }
    if (result == HW_TEST_OK) {
        result = expect_not_ack(dev, MISSING_I2C_ADDRESS, 65u, 0);
    }

    if (result == HW_TEST_OK) {
        result = test_chunk_lengths(dev, cfg.eeprom_addr);
    }

    cleanup_result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (cleanup_result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("PASS: I2C NACK handling and EEPROM chunk lengths 55..64 "
           "with original page restored\n");
    return HW_TEST_OK;
}
