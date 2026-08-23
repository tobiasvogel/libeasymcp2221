#include "mcp2221_gpio.h"
#include <string.h>

#include "mcp2221_internal_constants.h"
#include "mcp2221_internal.h"

// Internal helpers implemented in src/mcp2221.c (not part of the public API)

/* Python:
 * MCP2221_GPIO_ALTER_VALUE   = 1
 * PRESERVE      = 0
 */

#define MCP2221_GPIO_ALTER_VALUE 1
#define MCP2221_GPIO_PRESERVE_VALUE 0
#define MCP2221_GPIO_VALUE_LOW 0x00u
#define MCP2221_GPIO_VALUE_HIGH 0x01u
#define MCP2221_GPIO_VALUE_NOT_GPIO 0xEEu

static mcp2221_error_code_t decode_gpio_values(
	const uint8_t resp[MCP2221_PACKET_SIZE],
	int values[4]) {
	static const uint8_t value_offsets[4] = {
		MCP2221_GPIO_GET_RESP_GP0_VALUE,
		MCP2221_GPIO_GET_RESP_GP1_VALUE,
		MCP2221_GPIO_GET_RESP_GP2_VALUE,
		MCP2221_GPIO_GET_RESP_GP3_VALUE,
	};
	int decoded[4];

	for (int i = 0; i < 4; i++) {
		uint8_t raw = resp[value_offsets[i]];
		if (raw == MCP2221_GPIO_VALUE_NOT_GPIO)
			decoded[i] = -1;
		else if (raw == MCP2221_GPIO_VALUE_LOW ||
		         raw == MCP2221_GPIO_VALUE_HIGH)
			decoded[i] = raw;
		else
			return MCP2221_ERR_PROTOCOL;
	}

	memcpy(values, decoded, sizeof(decoded));
	return MCP2221_ERR_OK;
}

static int is_valid_gpio_write_value(int value) {
	return value == MCP2221_GPIO_KEEP || value == 0 || value == 1;
}

mcp2221_error_code_t mcp2221_gpio_write(mcp2221_t *dev, const mcp2221_gpio_write_t *wr) {
	if (!dev || !wr)
		return MCP2221_ERR_INVALID;
	if (!is_valid_gpio_write_value(wr->gp0) ||
	    !is_valid_gpio_write_value(wr->gp1) ||
	    !is_valid_gpio_write_value(wr->gp2) ||
	    !is_valid_gpio_write_value(wr->gp3))
		return MCP2221_ERR_INVALID;

	uint8_t buf[18] = {0};

	buf[0] = MCP2221_CMD_SET_GPIO_OUTPUT_VALUES;

	// GP0
	buf[2] = (wr->gp0 < 0) ? MCP2221_GPIO_PRESERVE_VALUE : MCP2221_GPIO_ALTER_VALUE;
	buf[3] = (wr->gp0 < 0) ? 0 : (wr->gp0 ? 1 : 0);

	// GP1
	buf[6] = (wr->gp1 < 0) ? MCP2221_GPIO_PRESERVE_VALUE : MCP2221_GPIO_ALTER_VALUE;
	buf[7] = (wr->gp1 < 0) ? 0 : (wr->gp1 ? 1 : 0);

	// GP2
	buf[10] = (wr->gp2 < 0) ? MCP2221_GPIO_PRESERVE_VALUE : MCP2221_GPIO_ALTER_VALUE;
	buf[11] = (wr->gp2 < 0) ? 0 : (wr->gp2 ? 1 : 0);

	// GP3
	buf[14] = (wr->gp3 < 0) ? MCP2221_GPIO_PRESERVE_VALUE : MCP2221_GPIO_ALTER_VALUE;
	buf[15] = (wr->gp3 < 0) ? 0 : (wr->gp3 ? 1 : 0);

	uint8_t resp[64];
	mcp2221_error_code_t err = mcp2221_send_cmd(dev, buf, sizeof(buf), resp);
	if (err)
		return err;

	// Python behavior: update cached GPIO out state for those that did not error, then raise on first error.
	// Cache is best-effort; if it can't be initialized, we still return success/failure based on the device reply.
	(void)mcp2221_internal_ensure_gpio_status(dev);
	if (wr->gp0 >= 0 && resp[3] != MCP2221_GPIO_VALUE_NOT_GPIO)
		mcp2221_internal_gpio_status_update_out(dev, 0, buf[3]);
	if (wr->gp1 >= 0 && resp[7] != MCP2221_GPIO_VALUE_NOT_GPIO)
		mcp2221_internal_gpio_status_update_out(dev, 1, buf[7]);
	if (wr->gp2 >= 0 && resp[11] != MCP2221_GPIO_VALUE_NOT_GPIO)
		mcp2221_internal_gpio_status_update_out(dev, 2, buf[11]);
	if (wr->gp3 >= 0 && resp[15] != MCP2221_GPIO_VALUE_NOT_GPIO)
		mcp2221_internal_gpio_status_update_out(dev, 3, buf[15]);

	if (wr->gp0 >= 0 && resp[3] == MCP2221_GPIO_VALUE_NOT_GPIO)
		return MCP2221_ERR_GPIO_MODE;
	else if (wr->gp1 >= 0 && resp[7] == MCP2221_GPIO_VALUE_NOT_GPIO)
		return MCP2221_ERR_GPIO_MODE;
	else if (wr->gp2 >= 0 && resp[11] == MCP2221_GPIO_VALUE_NOT_GPIO)
		return MCP2221_ERR_GPIO_MODE;
	else if (wr->gp3 >= 0 && resp[15] == MCP2221_GPIO_VALUE_NOT_GPIO)
		return MCP2221_ERR_GPIO_MODE;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_gpio_read(mcp2221_t *dev, int out_state[4]) {
	if (!dev || !out_state)
		return MCP2221_ERR_INVALID;

	uint8_t cmd[1] = {MCP2221_CMD_GET_GPIO_VALUES};
	uint8_t resp[64];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_transport(dev, cmd, 1, resp);
	if (err)
		return err;

	return decode_gpio_values(resp, out_state);
}

mcp2221_error_code_t mcp2221_gpio_read_mask(mcp2221_t *dev, int out_state[4], uint8_t *out_valid_mask) {
	if (!dev || !out_state || !out_valid_mask)
		return MCP2221_ERR_INVALID;

	uint8_t cmd[1] = {MCP2221_CMD_GET_GPIO_VALUES};
	uint8_t resp[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_transport(dev, cmd, 1, resp);
	if (err)
		return err;

	int decoded[4];
	err = decode_gpio_values(resp, decoded);
	if (err != MCP2221_ERR_OK)
		return err;

	uint8_t mask = 0;
	for (int i = 0; i < 4; i++) {
		if (decoded[i] >= 0)
			mask |= (uint8_t)(1u << i);
	}

	memcpy(out_state, decoded, sizeof(decoded));
	*out_valid_mask = mask;
	return MCP2221_ERR_OK;
}
