#include "mcp2221_pin.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "exceptions.h"

/*
 * Python mapping tables:
 * gp0_funcs = {...}
 * gp1_funcs = {...}
 * gp2_funcs = {...}
 * gp3_funcs = {...}
 *
 * We port these 1:1 as arrays.
 */

static const uint8_t gp0_funcs[] = {
	GPIO_FUNC_GPIO,		 /* GPIO */
	GPIO_FUNC_DEDICATED, /* (not used for GP0, but placeholder) */
	GPIO_FUNC_ADC,		 /* ALT0 */
	GPIO_FUNC_DAC,		 /* ALT1 */
	GPIO_FUNC_ALT_2		 /* ALT2: clock output */
};

static const uint8_t gp1_funcs[] = {GPIO_FUNC_GPIO, GPIO_FUNC_DEDICATED, GPIO_FUNC_ALT_0, GPIO_FUNC_ALT_1,
									GPIO_FUNC_ALT_2};

static const uint8_t gp2_funcs[] = {GPIO_FUNC_GPIO, GPIO_FUNC_DEDICATED, GPIO_FUNC_ALT_0, GPIO_FUNC_ALT_1,
									GPIO_FUNC_ALT_2};

static const uint8_t gp3_funcs[] = {GPIO_FUNC_GPIO, GPIO_FUNC_DEDICATED, GPIO_FUNC_ALT_0, GPIO_FUNC_ALT_1,
									GPIO_FUNC_ALT_2};

static inline const uint8_t *get_table(int pin) {
	switch (pin) {
		case MCP_GP0:
			return gp0_funcs;
		case MCP_GP1:
			return gp1_funcs;
		case MCP_GP2:
			return gp2_funcs;
		case MCP_GP3:
			return gp3_funcs;
		default:
			return NULL;
	}
}

static inline int get_sram_offset(int pin) {
	switch (pin) {
		case MCP_GP0:
			return SRAM_GP_SETTINGS_GP0;
		case MCP_GP1:
			return SRAM_GP_SETTINGS_GP1;
		case MCP_GP2:
			return SRAM_GP_SETTINGS_GP2;
		case MCP_GP3:
			return SRAM_GP_SETTINGS_GP3;
		default:
			return -1;
	}
}

/*
 * Equivalent to Python's:
 * new = (old & 0b10011111) | (func << 5)
 *
 * Because bits function = bits 6..5, use mask 0b1001 1111 = 0x9F.
 */
int mcp2221_set_pin_function(MCP2221 *dev, MCP_GPIO_Pin pin, MCP_PinFunction function) {
	if (!dev)
		return MCP_ERR_INVALID;
	if (pin < 0 || pin > 3)
		return MCP_ERR_INVALID;

	const uint8_t *table = get_table(pin);
	if (!table)
		return MCP_ERR_INVALID;

	uint8_t func_bits = table[function];

	// Step 1: Read entire SRAM settings
	uint8_t cmd = CMD_GET_SRAM_SETTINGS;
	uint8_t resp[64];

	mcp_err_t err = mcp2221_send_cmd(dev, &cmd, 1, resp);
	if (err != 0)
		return err;

	// Step 2: locate GPx config byte
	int off = 4 + get_sram_offset(pin); /* see Python: starting at byte 4 */
	if (off < 4)
		return MCP_ERR_INVALID;

	uint8_t oldval = resp[off];

	// Step 3: calculate new pin config
	uint8_t newval = (oldval & 0x9F) | (func_bits << 5);

	// Step 4: send CMD_SET_SRAM_SETTINGS

	uint8_t buf[64] = {0};
	buf[0] = CMD_SET_SRAM_SETTINGS;

	/*
	 * Bit 7 = ALTER_GPIO_CONF
	 * Lower 7 bits = new configuration byte.
	 *
	 * Python:
	 * buf[4 + pin_offset] = ALTER | newvalue
	 */

	buf[off] = ALTER_GPIO_CONF | (newval & 0x1F);

	uint8_t r2[64];
	err = mcp2221_send_cmd(dev, buf, sizeof(buf), r2);
	if (err != 0)
		return err;

	return MCP_ERR_OK;
}
