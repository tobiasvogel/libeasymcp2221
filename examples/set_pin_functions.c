#include <stdio.h>
#include <string.h>

#include "mcp2221_constants.h"
#include "mcp2221.h"
#include "mcp2221_gpio.h"
#include "mcp2221_pin.h"

/*
 * Example mirroring EasyMCP2221's Python docs:
 *
 *   mcp.set_pin_function(
 *       gp0 = "GPIO_IN",
 *       gp1 = "GPIO_OUT", out1 = True,
 *       gp2 = "ADC",
 *       gp3 = "LED_I2C")
 *
 * Notes about function meanings:
 * - This C port uses a generic enum (DEDICATED/ALT0/ALT1/ALT2). The actual meaning depends on the pin:
 *   - GP2: ALT0=ADC, ALT1=DAC, DEDICATED=USBCFG
 *   - GP3: ALT0=ADC, ALT1=DAC, DEDICATED=LED_I2C
 *   - GP1: DEDICATED=CLK_OUT, ALT0=ADC, ALT1=LED_UTX, ALT2=IOC
 *   - GP0: DEDICATED=SSPND, ALT0=LED_URX
 */

int main(void) {
	mcp2221_t *dev = mcp2221_open(MCP2221_DEV_DEFAULT_VID, MCP2221_DEV_DEFAULT_PID, 0, NULL, 500, 3, 0, 0);
	if (!dev) {
		fprintf(stderr, "Failed to open MCP2221.\n");
		return 1;
	}

	mcp2221_pin_functions_t cfg;
	memset(&cfg, 0, sizeof(cfg));

	// Preserve everything by default
	for (int i = 0; i < 4; i++) {
		cfg.gp[i] = MCP2221_PIN_FUNC_KEEP;
		cfg.out[i] = 0;
	}

	cfg.gp[0] = MCP2221_PIN_FUNC_GPIO_IN;
	cfg.gp[1] = MCP2221_PIN_FUNC_GPIO_OUT;
	cfg.out[1] = 1; // out1=True is only valid with GPIO_OUT (Python constraint)
	cfg.gp[2] = MCP2221_PIN_FUNC_ALT0;		// GP2 ALT0 = ADC
	cfg.gp[3] = MCP2221_PIN_FUNC_DEDICATED; // GP3 DEDICATED = LED_I2C

	int r = mcp2221_pin_set_functions(dev, &cfg);
	if (r != MCP2221_ERR_OK) {
		fprintf(stderr, "mcp2221_pin_set_functions failed: %d\n", r);
		mcp2221_close(dev);
		return 2;
	}

	// Read current pin logic states (non-GPIO pins are reported as -1).
	int state[4];
	uint8_t mask = 0;
	r = mcp2221_gpio_read_mask(dev, state, &mask);
	if (r == MCP2221_ERR_OK) {
		printf("GPIO valid mask: 0x%02X\n", mask);
		printf("GP0=%d GP1=%d GP2=%d GP3=%d\n", state[0], state[1], state[2], state[3]);
	}

	mcp2221_close(dev);
	return 0;
}

