#ifndef MCP2221_GPIO_H
#define MCP2221_GPIO_H

#include <stdint.h>

#include "mcp2221.h"

/* Special value for mcp2221_gpio_write_t fields: preserve the current output value.
 * This is intentionally separate from MCP2221_CONFIG_KEEP, which belongs to SRAM configuration helpers.
 */
#define MCP2221_GPIO_KEEP (-1)

typedef struct {
	int gp0;
	int gp1;
	int gp2;
	int gp3;
} MCP2221_GPIO_Write;
typedef MCP2221_GPIO_Write mcp2221_gpio_write_t;

/**
 * Write GPIO output pins.
 *
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on error.
 *
 * Values:
 *   MCP2221_GPIO_KEEP = preserve
 *   0                 = drive low
 *   1                 = drive high
 */
mcp2221_error_code_t mcp2221_gpio_write(MCP2221 *dev, const MCP2221_GPIO_Write *wr);

/**
 * Read GPIO state.
 *
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on error.
 * Pin states are reported through out_state:
 *
 *   -1 = pin is NOT GPIO (e.g. DAC/ADC)
 *    0 = logic low
 *    1 = logic high
 */
mcp2221_error_code_t mcp2221_gpio_read(MCP2221 *dev, int out_state[4]);

/**
 * Read GPIO state with a validity mask.
 *
 * This is the closest C equivalent to Python's `GPIO_read()` which returns `None`
 * for pins that are not configured as GPIO. Here:
 * - `out_state[i]` is -1 if the pin is not GPIO, else 0/1 for logic level.
 * - `out_valid_mask` bit i is 1 if pin i is GPIO, else 0.
 *
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on error.
 */
mcp2221_error_code_t mcp2221_gpio_read_mask(MCP2221 *dev, int out_state[4], uint8_t *out_valid_mask);

#endif	// MCP2221_GPIO_H
