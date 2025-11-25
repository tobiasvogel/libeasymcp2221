#ifndef MCP2221_PIN_H
#define MCP2221_PIN_H

#include <stdint.h>

#include "mcp2221.h"

/* MCP2221 GP pin numbers */
typedef enum { MCP_GP0 = 0, MCP_GP1 = 1, MCP_GP2 = 2, MCP_GP3 = 3 } MCP_GPIO_Pin;

/* High-level pin functions (Python API compatible) */
typedef enum {
	MCP_PIN_FUNC_GPIO = 0,
	MCP_PIN_FUNC_DEDICATED = 1,
	MCP_PIN_FUNC_ALT0 = 2,
	MCP_PIN_FUNC_ALT1 = 3,
	MCP_PIN_FUNC_ALT2 = 4
} MCP_PinFunction;

/* Set pin function exactly like Python's Device.set_pin_function() */
int mcp2221_set_pin_function(MCP2221 *dev, MCP_GPIO_Pin pin, MCP_PinFunction function);

#endif	// MCP2221_PIN_H