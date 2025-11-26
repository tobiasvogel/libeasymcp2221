#include "mcp2221_gpio.h"
#include "constants.h"

#include <string.h>

/* Python:
 * ALTER_VALUE   = 1
 * PRESERVE      = 0
 * GPIO_ERROR    = 0xEE
 */

#define ALTER_VALUE 1
#define PRESERVE_VALUE 0
#define GPIO_ERROR 0xEE

int mcp2221_gpio_write(MCP2221 *dev, const MCP2221_GPIO_Write *wr) {
	uint8_t buf[18] = {0};

	buf[0] = CMD_SET_GPIO_OUTPUT_VALUES;

	// GP0
	buf[2] = (wr->gp0 < 0) ? PRESERVE_VALUE : ALTER_VALUE;
	buf[3] = (wr->gp0 < 0) ? 0 : (wr->gp0 ? 1 : 0);

	// GP1
	buf[6] = (wr->gp1 < 0) ? PRESERVE_VALUE : ALTER_VALUE;
	buf[7] = (wr->gp1 < 0) ? 0 : (wr->gp1 ? 1 : 0);

	// GP2
	buf[10] = (wr->gp2 < 0) ? PRESERVE_VALUE : ALTER_VALUE;
	buf[11] = (wr->gp2 < 0) ? 0 : (wr->gp2 ? 1 : 0);

	// GP3
	buf[14] = (wr->gp3 < 0) ? PRESERVE_VALUE : ALTER_VALUE;
	buf[15] = (wr->gp3 < 0) ? 0 : (wr->gp3 ? 1 : 0);

	uint8_t resp[64];
	int err = mcp2221_send_cmd(dev, buf, sizeof(buf), resp);
	if (err)
		return err;

	// Error checking: identical to Python
	if (wr->gp0 >= 0 && resp[3] == GPIO_ERROR)
		return MCP_ERR_GPIO_MODE;
	if (wr->gp1 >= 0 && resp[7] == GPIO_ERROR)
		return MCP_ERR_GPIO_MODE;
	if (wr->gp2 >= 0 && resp[11] == GPIO_ERROR)
		return MCP_ERR_GPIO_MODE;
	if (wr->gp3 >= 0 && resp[15] == GPIO_ERROR)
		return MCP_ERR_GPIO_MODE;

	return 0;
}

int mcp2221_gpio_read(MCP2221 *dev, int out_state[4]) {
	uint8_t cmd[1] = {CMD_GET_GPIO_VALUES};
	uint8_t resp[64];

	int err = mcp2221_send_cmd(dev, cmd, 1, resp);
	if (err)
		return err;

	out_state[0] = (resp[2] == GPIO_ERROR) ? -1 : resp[2];
	out_state[1] = (resp[4] == GPIO_ERROR) ? -1 : resp[4];
	out_state[2] = (resp[6] == GPIO_ERROR) ? -1 : resp[6];
	out_state[3] = (resp[8] == GPIO_ERROR) ? -1 : resp[8];

	return 0;
}
