#include <stdio.h>

#include "mcp2221.h"
#include "mcp2221_gpio.h"
#include "mcp2221_pin.h"

static void dump_states(const int state[4], uint8_t mask) {
	for (int i = 0; i < 4; i++) {
		const char *mode = (mask & (1u << i)) ? "GPIO" : "ALT";
		int v = state[i];
		const char *val = (v < 0) ? "n/a" : (v ? "HIGH" : "LOW");
		printf("GP%d: mode=%s value=%s\n", i, mode, val);
	}
}

int main(void) {
	MCP2221 *dev = mcp2221_open_simple(0x04D8, 0x00DD, 0, NULL, 100000);
	if (!dev) {
		fprintf(stderr, "Failed to open MCP2221\n");
		return 1;
	}

	// Configure pins similar to Python GPIO_write/GPIO_read usage:
	// GP0: GPIO_OUT (set HIGH), GP1: GPIO_IN, GP2: GPIO_OUT (set LOW), GP3: GPIO_IN.
	MCP2221_PinFunctions cfg = {0};
	cfg.gp[0] = MCP_PIN_FUNC_GPIO_OUT;
	cfg.gp[1] = MCP_PIN_FUNC_GPIO_IN;
	cfg.gp[2] = MCP_PIN_FUNC_GPIO_OUT;
	cfg.gp[3] = MCP_PIN_FUNC_GPIO_IN;
	cfg.out[0] = 1;
	cfg.out[2] = 0;

	if (mcp2221_set_pin_functions(dev, &cfg) != MCP_ERR_OK) {
		fprintf(stderr, "Failed to set pin functions\n");
		mcp2221_close(dev);
		return 1;
	}

	// Drive new values: keep GP1/GP3 unchanged (-1), set GP0=HIGH, GP2=LOW.
	MCP2221_GPIO_Write wr = {.gp0 = 1, .gp1 = -1, .gp2 = 0, .gp3 = -1};
	if (mcp2221_gpio_write(dev, &wr) != MCP_ERR_OK) {
		fprintf(stderr, "GPIO_write failed\n");
		mcp2221_close(dev);
		return 1;
	}

	int state[4] = {-1, -1, -1, -1};
	uint8_t mask = 0;
	if (mcp2221_gpio_read_mask(dev, state, &mask) != MCP_ERR_OK) {
		fprintf(stderr, "GPIO_read failed\n");
		mcp2221_close(dev);
		return 1;
	}

	dump_states(state, mask);

	mcp2221_close(dev);
	return 0;
}
