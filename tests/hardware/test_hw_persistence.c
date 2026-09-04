#include "hardware_test.h"

#include <stdio.h>
#include <string.h>

#define FLASH_GP0_OFFSET 0u
#define FLASH_GP1_OFFSET 1u
#define FLASH_GP2_OFFSET 2u
#define FLASH_GP3_OFFSET 3u
#define FLASH_CHIP_SETTINGS_USED 18u
#define FLASH_GP_SETTINGS_USED 4u

/*
 * Persistent GP-byte encoding used by the MCP2221:
 *   bit 4: output value
 *   bit 3: direction (1=input)
 *   bits 2..0: function (0=GPIO)
 */
#define GP_GPIO_INPUT 0x08u
#define GP_GPIO_OUTPUT_LOW 0x00u

static int configure_persistent_test_state(mcp2221_t *dev)
{
    const mcp2221_pin_functions_t cfg = {
        {
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_OUT,
            MCP2221_PIN_FUNC_GPIO_IN
        },
        {0, 0, 0, 0}
    };
    mcp2221_error_code_t rc;

    HW_TEST_RETRY_TIMEOUT_ONCE(
        rc, mcp2221_pin_set_functions(dev, &cfg));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("configuring persistence test GPIOs", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

static int verify_persisted_gp_settings(const mcp2221_flash_settings_t *st)
{
    const uint8_t expected[4] = {
        GP_GPIO_INPUT,
        GP_GPIO_INPUT,
        GP_GPIO_OUTPUT_LOW,
        GP_GPIO_INPUT
    };
    const uint8_t actual[4] = {
        st->gp_settings[FLASH_GP0_OFFSET],
        st->gp_settings[FLASH_GP1_OFFSET],
        st->gp_settings[FLASH_GP2_OFFSET],
        st->gp_settings[FLASH_GP3_OFFSET]
    };
    size_t i;

    if (memcmp(expected, actual, sizeof(expected)) == 0) {
        return HW_TEST_OK;
    }

    fprintf(stderr, "persisted GP settings mismatch:\n");
    for (i = 0; i < sizeof(expected); ++i) {
        fprintf(stderr,
                "  GP%zu: expected 0x%02x, got 0x%02x\n",
                i,
                (unsigned int)expected[i],
                (unsigned int)actual[i]);
    }
    return HW_TEST_FAILED;
}

static int restore_flash_settings(mcp2221_t *dev,
                                  const mcp2221_flash_settings_t *original)
{
    mcp2221_flash_settings_t restored;
    mcp2221_error_code_t rc;

    rc = mcp2221_flash_write(
        dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, original->chip_settings);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("restoring chip-settings flash", rc);
        return HW_TEST_FAILED;
    }

    rc = mcp2221_flash_write(
        dev, MCP2221_FLASH_DATA_GP_SETTINGS, original->gp_settings);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("restoring GP-settings flash", rc);
        return HW_TEST_FAILED;
    }

    HW_TEST_RETRY_TIMEOUT_ONCE(
        rc, mcp2221_flash_get_settings(dev, &restored));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("reading restored flash settings", rc);
        return HW_TEST_FAILED;
    }

    if (memcmp(original->chip_settings, restored.chip_settings,
               FLASH_CHIP_SETTINGS_USED) != 0) {
        size_t i;

        fprintf(stderr, "restored chip settings do not match original fields\n");
        for (i = 0; i < FLASH_CHIP_SETTINGS_USED; ++i) {
            if (original->chip_settings[i] != restored.chip_settings[i]) {
                fprintf(stderr,
                        "  chip offset %zu: expected 0x%02x, got 0x%02x\n",
                        i,
                        (unsigned int)original->chip_settings[i],
                        (unsigned int)restored.chip_settings[i]);
            }
        }
        return HW_TEST_FAILED;
    }

    if (memcmp(original->gp_settings, restored.gp_settings,
               FLASH_GP_SETTINGS_USED) != 0) {
        size_t i;

        fprintf(stderr, "restored GP settings do not match original fields\n");
        for (i = 0; i < FLASH_GP_SETTINGS_USED; ++i) {
            if (original->gp_settings[i] != restored.gp_settings[i]) {
                fprintf(stderr,
                        "  GP offset %zu: expected 0x%02x, got 0x%02x\n",
                        i,
                        (unsigned int)original->gp_settings[i],
                        (unsigned int)restored.gp_settings[i]);
            }
        }
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

int main(void)
{
    hw_test_config_t cfg;
    mcp2221_flash_settings_t original;
    mcp2221_flash_settings_t persisted;
    mcp2221_t *dev;
    mcp2221_error_code_t rc;
    int result;
    int restore_result;
    int cleanup_result;
    int original_saved = 0;

    if (hw_test_load_config(&cfg) != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    result = hw_test_open(&cfg, &dev);
    if (result != HW_TEST_OK) {
        return result;
    }

    HW_TEST_RETRY_TIMEOUT_ONCE(
        rc, mcp2221_flash_get_settings(dev, &original));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("saving original flash settings", rc);
        result = HW_TEST_FAILED;
        goto cleanup;
    }
    original_saved = 1;

    result = configure_persistent_test_state(dev);
    if (result != HW_TEST_OK) {
        goto restore;
    }

    rc = mcp2221_flash_save_config(dev);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("persisting runtime configuration", rc);
        result = HW_TEST_FAILED;
        goto restore;
    }

    HW_TEST_RETRY_TIMEOUT_ONCE(
        rc, mcp2221_flash_get_settings(dev, &persisted));
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("reading persisted flash settings", rc);
        result = HW_TEST_FAILED;
        goto restore;
    }

    result = verify_persisted_gp_settings(&persisted);

restore:
    restore_result = HW_TEST_OK;
    if (original_saved) {
        restore_result = restore_flash_settings(dev, &original);
    }
    if (restore_result != HW_TEST_OK) {
        result = HW_TEST_FAILED;
    }

cleanup:
    cleanup_result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (cleanup_result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("PASS: runtime GPIO configuration persisted to flash and original "
           "chip/GP settings restored\n");
    return HW_TEST_OK;
}
