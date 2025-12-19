#ifndef MCP2221_PIN_H
#define MCP2221_PIN_H

#include <stdint.h>

#include "mcp2221.h"

/* MCP2221 GP pin numbers */
typedef enum { MCP_GP0 = 0, MCP_GP1 = 1, MCP_GP2 = 2, MCP_GP3 = 3 } MCP_GPIO_Pin;

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
} MCP_PinFunction;

typedef struct {
	// Desired pin functions. Use MCP_PIN_FUNC_KEEP to preserve.
	MCP_PinFunction gp[4];

	// Output values for GP0..GP3 if and only if the corresponding gp[i] is MCP_PIN_FUNC_GPIO_OUT.
	// Python rules:
	// - outX=True is only valid if gpX == GPIO_OUT, otherwise ValueError.
	// - outX=False is always allowed.
	int out[4]; /* 0 or 1 */
} MCP2221_PinFunctions;

/* Set pin function exactly like Python's Device.set_pin_function() */
int mcp2221_set_pin_function(MCP2221 *dev, MCP_GPIO_Pin pin, MCP_PinFunction function);

/**
 * Configure multiple pins at once, mirroring EasyMCP2221's `Device.set_pin_function()` behaviour.
 *
 * - `cfg->gp[i] == MCP_PIN_FUNC_KEEP` preserves that pin's function.
 * - `cfg->out[i]` is only meaningful if `cfg->gp[i] == MCP_PIN_FUNC_GPIO_OUT`.
 *   If `cfg->out[i] == 1` for any other gp[i], this returns `MCP_ERR_INVALID`.
 *
 * Note: The "meaning" of DEDICATED/ALT0/ALT1/ALT2 depends on the pin (GP0..GP3), just like in Python.
 */
int mcp2221_set_pin_functions(MCP2221 *dev, const MCP2221_PinFunctions *cfg);

#endif	// MCP2221_PIN_H
