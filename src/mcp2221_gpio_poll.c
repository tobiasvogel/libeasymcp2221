#include "mcp2221_gpio_poll.h"

#include <string.h>

#include "constants.h"

#define GPIO_ERROR 0xEE

void mcp2221_gpio_poll_init(MCP_GPIO_PollState *st) {
	for (int i = 0; i < 4; i++)
		st->prev[i] = -2; /* -2 = uninitialized special value */
	st->initialized = 0;
}

int mcp2221_gpio_poll(MCP2221 *dev, MCP_GPIO_PollState *st, MCP_GPIO_Change out[4]) {
	uint8_t cmd = CMD_GET_GPIO_VALUES;
	uint8_t resp[PACKET_SIZE];

	mcp_err_t err = mcp2221_send_cmd(dev, &cmd, 1, resp);
	if (err)
		return err;

	/* extract GPIO states, Python uses offsets:
	   GP0 = resp[2], GP1 = resp[4], GP2 = resp[6], GP3 = resp[8] */
	int now[4];
	now[0] = (resp[2] == GPIO_ERROR) ? -1 : resp[2];
	now[1] = (resp[4] == GPIO_ERROR) ? -1 : resp[4];
	now[2] = (resp[6] == GPIO_ERROR) ? -1 : resp[6];
	now[3] = (resp[8] == GPIO_ERROR) ? -1 : resp[8];

	// first call: initialize state, no changes reported
	if (!st->initialized) {
		for (int i = 0; i < 4; i++) {
			st->prev[i] = now[i];
			out[i].old_value = now[i];
			out[i].new_value = now[i];
			out[i].changed = 0;
		}
		st->initialized = 1;
		return 0;
	}

	// detect changes
	for (int i = 0; i < 4; i++) {
		if (now[i] != st->prev[i]) {
			out[i].old_value = st->prev[i];
			out[i].new_value = now[i];
			out[i].changed = 1;
		} else {
			out[i].old_value = st->prev[i];
			out[i].new_value = now[i];
			out[i].changed = 0;
		}
	}

	// store state
	for (int i = 0; i < 4; i++)
		st->prev[i] = now[i];

	return 0;
}
