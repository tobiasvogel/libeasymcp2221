/**
 * @file mcp2221_gpio.h
 * @brief Direct GPIO input and output helpers.
 */

#ifndef MCP2221_GPIO_H
#define MCP2221_GPIO_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Preserve the current GPIO output value.
 *
 * Use this value in a member of mcp2221_gpio_write_t to leave the
 * corresponding output unchanged.
 *
 * @note This sentinel is separate from MCP2221_CONFIG_KEEP, which belongs to
 *       the SRAM configuration API.
 */
#define MCP2221_GPIO_KEEP (-1)

/**
 * @brief Per-pin GPIO output update request.
 *
 * Each member controls one MCP2221 GP pin. Valid values are
 * MCP2221_GPIO_KEEP to preserve the current output value, 0 to drive the pin
 * low, and 1 to drive it high.
 */
typedef struct {
	int gp0; /**< Requested output for GP0: MCP2221_GPIO_KEEP, 0, or 1. */
	int gp1; /**< Requested output for GP1: MCP2221_GPIO_KEEP, 0, or 1. */
	int gp2; /**< Requested output for GP2: MCP2221_GPIO_KEEP, 0, or 1. */
	int gp3; /**< Requested output for GP3: MCP2221_GPIO_KEEP, 0, or 1. */
} mcp2221_gpio_write_t;

/**
 * @brief Update GPIO output values.
 *
 * Only members whose value is 0 or 1 are changed; members set to
 * MCP2221_GPIO_KEEP preserve the current output value.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] wr Per-pin output update request. Must not be `NULL`.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments or member values, MCP2221_ERR_GPIO_MODE when a requested
 *         pin cannot be written as GPIO, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @note The library updates its cached GPIO output state for pins successfully
 *       accepted by the device.
 */
MCP2221_API mcp2221_error_code_t mcp2221_gpio_write(mcp2221_t *dev, const mcp2221_gpio_write_t *wr);

/**
 * @brief Read the current state of GP0 through GP3.
 *
 * Each element of @p out_state corresponds to the same-numbered GP pin.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] out_state Four-element array receiving pin states:
 *                       - -1 when the pin is not configured as GPIO
 *                       - 0 for logic low
 *                       - 1 for logic high
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_gpio_read(mcp2221_t *dev, int out_state[4]);

/**
 * @brief Read GPIO state and report which GP pins are configured as GPIO.
 *
 * This mirrors the EasyMCP2221 GPIO-read semantics while providing an
 * explicit validity mask in C.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] out_state Four-element array receiving -1 for non-GPIO pins,
 *                       otherwise 0 or 1 for the sampled logic level.
 * @param[out] out_valid_mask Receives a bit mask where bit 0 through bit 3
 *                            correspond to GP0 through GP3. A set bit means
 *                            the corresponding pin is configured as GPIO.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_gpio_read()
 */
MCP2221_API mcp2221_error_code_t mcp2221_gpio_read_mask(mcp2221_t *dev, int out_state[4], uint8_t *out_valid_mask);

MCP2221_END_DECLS
#endif	// MCP2221_GPIO_H
