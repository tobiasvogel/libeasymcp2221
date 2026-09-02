#ifndef LIBEASYMCP2221_HARDWARE_TEST_H
#define LIBEASYMCP2221_HARDWARE_TEST_H

#include <stdint.h>

#include "libeasymcp2221.h"

#define HW_TEST_OK 0
#define HW_TEST_FAILED 1
#define HW_TEST_SKIPPED 77

typedef struct {
    uint16_t vid;
    uint16_t pid;
    int devnum;
    const char *serial;
    uint8_t eeprom_addr;
} hw_test_config_t;

/*
 * Load test-fixture selection from the environment.
 *
 * Defaults:
 *   LIBEASYMCP2221_HW_VID       0x04d8
 *   LIBEASYMCP2221_HW_PID       0x00dd
 *   LIBEASYMCP2221_HW_DEVNUM    0
 *   LIBEASYMCP2221_HW_SERIAL    unset
 *   LIBEASYMCP2221_HW_EEPROM_ADDR 0x50
 */
int hw_test_load_config(hw_test_config_t *cfg);

/*
 * Open the selected MCP2221.
 *
 * Returns HW_TEST_OK on success, HW_TEST_SKIPPED when no matching MCP2221 is
 * present, and HW_TEST_FAILED for all other errors.
 */
int hw_test_open(const hw_test_config_t *cfg, mcp2221_t **out_dev);

/* Put all four GP pins into the non-driving GPIO-input state. */
int hw_test_safe_state(mcp2221_t *dev);

/* Sleep long enough for physical signal paths to settle. */
void hw_test_sleep_ms(unsigned int milliseconds);

void hw_test_print_error(const char *operation, mcp2221_error_code_t rc);

#endif /* LIBEASYMCP2221_HARDWARE_TEST_H */
