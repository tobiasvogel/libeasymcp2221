#ifndef MCP2221_PIN_H
#define MCP2221_PIN_H

#include <stdint.h>

#include "mcp2221.h"

/* MCP2221 GP pin numbers */
typedef enum { MCP_GP0 = 0, MCP_GP1 = 1, MCP_GP2 = 2, MCP_GP3 = 3 } mcp2221_gpio_pin_t;

/* High-level pin functions (Python API compatible) */
typedef enum {
	// Special value: do not alter this pin (Python: gpX=None)
	MCP_PIN_FUNC_KEEP = -1,

	MCP_PIN_FUNC_DEDICATED = 0,
	MCP_PIN_FUNC_ALT0 = 1,
	MCP_PIN_FUNC_ALT1 = 2,
	MCP_PIN_FUNC_ALT2 = 3,
	MCP_PIN_FUNC_GPIO_IN = 4,
	MCP_PIN_FUNC_GPIO_OUT = 5
} mcp2221_pin_function_t;

typedef struct {
	// Desired pin functions. Use MCP_PIN_FUNC_KEEP to preserve.
	mcp2221_pin_function_t gp[4];

	// Output values for GP0..GP3 if and only if the corresponding gp[i] is MCP_PIN_FUNC_GPIO_OUT.
	// Python rules:
	// - outX=True is only valid if gpX == GPIO_OUT, otherwise ValueError.
	// - outX=False is always allowed.
	int out[4]; /* 0 or 1 */
} mcp2221_pin_functions_t;

/* Preferred verb-object names. */
MCP2221_API mcp2221_error_code_t mcp2221_pin_set_function(mcp2221_t *dev, mcp2221_gpio_pin_t pin, mcp2221_pin_function_t function);
MCP2221_API mcp2221_error_code_t mcp2221_pin_set_functions(mcp2221_t *dev, const mcp2221_pin_functions_t *cfg);

#endif	// MCP2221_PIN_H
