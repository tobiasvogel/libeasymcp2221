#ifndef MCP2221_GPIO_POLL_H
#define MCP2221_GPIO_POLL_H

#include <stdint.h>

#include "mcp2221.h"

// State of a single GPIO-Pin
typedef struct {
	int old_value; /* -1 = unknown (not GPIO), 0/1 = valid */
	int new_value;
	int changed; /* 0 = no change, 1 = changed */
} MCP_GPIO_Change;

// Polling-Object
typedef struct {
	int prev[4]; /* previous GP0..GP3 states */
	int initialized;
} MCP_GPIO_PollState;

// Initialize
void mcp2221_gpio_poll_init(MCP_GPIO_PollState *st);

// Poll and notify on change
int mcp2221_gpio_poll(MCP2221 *dev, MCP_GPIO_PollState *st, MCP_GPIO_Change out[4]);

#endif	// MCP2221_GPIO_POLL_H