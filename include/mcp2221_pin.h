/**
 * @file mcp2221_pin.h
 * @brief High-level MCP2221 GP pin-function configuration.
 */

#ifndef MCP2221_PIN_H
#define MCP2221_PIN_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief MCP2221 general-purpose pin identifiers.
 */
typedef enum {
	MCP2221_GPIO_PIN_GP0 = 0, /**< GP0. */
	MCP2221_GPIO_PIN_GP1 = 1, /**< GP1. */
	MCP2221_GPIO_PIN_GP2 = 2, /**< GP2. */
	MCP2221_GPIO_PIN_GP3 = 3  /**< GP3. */
} mcp2221_gpio_pin_t;

/**
 * @brief High-level pin functions.
 *
 * The ALT function values represent the MCP2221 alternate-function selector,
 * but not every alternate function is valid on every GP pin.
 */
typedef enum {
	/** Preserve the current configuration of this pin. */
	MCP2221_PIN_FUNC_KEEP = -1,

	/** Select the pin's dedicated MCP2221 function. */
	MCP2221_PIN_FUNC_DEDICATED = 0,

	/** Select alternate function 0 for the pin. */
	MCP2221_PIN_FUNC_ALT0 = 1,

	/** Select alternate function 1 where supported. */
	MCP2221_PIN_FUNC_ALT1 = 2,

	/** Select alternate function 2; supported only on GP1. */
	MCP2221_PIN_FUNC_ALT2 = 3,

	/** Configure the pin as a GPIO input. */
	MCP2221_PIN_FUNC_GPIO_IN = 4,

	/** Configure the pin as a GPIO output. */
	MCP2221_PIN_FUNC_GPIO_OUT = 5
} mcp2221_pin_function_t;

/**
 * @brief Batch configuration for GP0 through GP3.
 *
 * Array index 0 through 3 corresponds to GP0 through GP3.
 */
typedef struct {
	/**
	 * @brief Desired function for each GP pin.
	 *
	 * Use MCP2221_PIN_FUNC_KEEP to leave a pin unchanged.
	 */
	mcp2221_pin_function_t gp[4];

	/**
	 * @brief GPIO output value associated with each GP pin.
	 *
	 * Every entry must be either 0 or 1. A value of 1 is valid only when the
	 * corresponding @ref gp entry is MCP2221_PIN_FUNC_GPIO_OUT. For other pin
	 * functions, including MCP2221_PIN_FUNC_KEEP, the value must be 0.
	 */
	int out[4];
} mcp2221_pin_functions_t;

/**
 * @brief Set the function of one GP pin.
 *
 * MCP2221_PIN_FUNC_KEEP is accepted and leaves the selected pin unchanged.
 * For functions that actively configure the pin, the helper writes a complete
 * GP configuration. GPIO outputs are initialized low because this function
 * has no output-value parameter.
 *
 * Supported functions by pin:
 * - GP0: DEDICATED, ALT0, GPIO_IN, GPIO_OUT
 * - GP1: DEDICATED, ALT0, ALT1, ALT2, GPIO_IN, GPIO_OUT
 * - GP2: DEDICATED, ALT0, ALT1, GPIO_IN, GPIO_OUT
 * - GP3: DEDICATED, ALT0, ALT1, GPIO_IN, GPIO_OUT
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] pin GP pin to configure.
 * @param[in] function Requested high-level pin function.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid pin
 *         or unsupported function, or another mcp2221_error_code_t value on
 *         failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_pin_set_function(mcp2221_t *dev, mcp2221_gpio_pin_t pin, mcp2221_pin_function_t function);

/**
 * @brief Configure the functions of GP0 through GP3 in one operation.
 *
 * Pins whose @ref mcp2221_pin_functions_t::gp entry is
 * MCP2221_PIN_FUNC_KEEP are preserved. For GPIO outputs, the corresponding
 * @ref mcp2221_pin_functions_t::out entry selects the initial output value.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] cfg Four-pin configuration. Must not be `NULL`.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid output
 *         values or unsupported pin/function combinations, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @note Every @ref mcp2221_pin_functions_t::out entry must be 0 or 1. A value
 *       of 1 is allowed only for a pin configured as
 *       MCP2221_PIN_FUNC_GPIO_OUT.
 */
MCP2221_API mcp2221_error_code_t mcp2221_pin_set_functions(mcp2221_t *dev, const mcp2221_pin_functions_t *cfg);

MCP2221_END_DECLS
#endif	// MCP2221_PIN_H
