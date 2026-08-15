#include "mcp2221_gpio_poll.h"

#include <time.h>
#include <stdio.h>

#include "mcp2221_internal_constants.h"
#include "mcp2221_internal.h"

#define MCP2221_GPIO_ERROR 0xEE

static void decode_gpio_values(const uint8_t resp[MCP2221_PACKET_SIZE], int values[4]) {
	values[0] = (resp[MCP2221_GPIO_GET_RESP_GP0_VALUE] == MCP2221_GPIO_ERROR)
		? -1 : resp[MCP2221_GPIO_GET_RESP_GP0_VALUE];
	values[1] = (resp[MCP2221_GPIO_GET_RESP_GP1_VALUE] == MCP2221_GPIO_ERROR)
		? -1 : resp[MCP2221_GPIO_GET_RESP_GP1_VALUE];
	values[2] = (resp[MCP2221_GPIO_GET_RESP_GP2_VALUE] == MCP2221_GPIO_ERROR)
		? -1 : resp[MCP2221_GPIO_GET_RESP_GP2_VALUE];
	values[3] = (resp[MCP2221_GPIO_GET_RESP_GP3_VALUE] == MCP2221_GPIO_ERROR)
		? -1 : resp[MCP2221_GPIO_GET_RESP_GP3_VALUE];
}

static double wall_time_seconds(void) {
#if defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
#else
	return (double)time(NULL);
#endif
}

void mcp2221_gpio_poll_init(mcp2221_gpio_poll_state_t *st) {
	if (!st)
		return;

	for (int i = 0; i < 4; i++)
		st->prev[i] = -2; /* -2 = uninitialized special value */
	st->initialized = 0;
	st->last_time = 0.0;
	st->filter_mask = 0; /* 0 = accept all, like Python's default [] */
}

void mcp2221_gpio_poll_set_filter_mask(mcp2221_gpio_poll_state_t *st, uint16_t mask) {
	if (!st)
		return;
	st->filter_mask = mask;
}

mcp2221_error_code_t mcp2221_gpio_poll(
    mcp2221_t *dev,
    mcp2221_gpio_poll_state_t *st,
    mcp2221_gpio_change_t out[4]
) {
	if (!dev || !st || !out)
		return MCP2221_ERR_INVALID;

	uint8_t cmd = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t resp[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_transport(dev, &cmd, 1, resp);
	if (err)
		return err;

	int now[4];
	decode_gpio_values(resp, now);

	// first call: initialize state, no changes reported
	if (!st->initialized) {
		for (int i = 0; i < 4; i++) {
			st->prev[i] = now[i];
			out[i].old_value = now[i];
			out[i].new_value = now[i];
			out[i].changed = 0;
		}
		st->initialized = 1;
		return MCP2221_ERR_OK;
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

	return MCP2221_ERR_OK;
}

static int mask_allows(uint16_t mask, int pin, mcp2221_gpio_event_type_t type) {
	if (mask == 0)
		return 1; /* 0 = all events, mirroring Python filter=[] */
	int bit = pin * 2 + (type == MCP2221_GPIO_EVENT_FALL ? 1 : 0);
	return (mask & (1u << bit)) != 0;
}

int mcp2221_gpio_poll_events(mcp2221_t *dev, mcp2221_gpio_poll_state_t *st, const uint16_t *filter_mask_opt,
							mcp2221_gpio_event_t *out_events, size_t max_events) {
	if (!dev || !st || (!out_events && max_events > 0))
		return MCP2221_ERR_INVALID;

	uint8_t cmd = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t resp[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_transport(dev, &cmd, 1, resp);
	if (err)
		return err;

	int now[4];
	decode_gpio_values(resp, now);

	double current_time = wall_time_seconds();

	// Update filter only if caller provided it; otherwise preserve the last selection (Python behavior).
	if (filter_mask_opt)
		st->filter_mask = *filter_mask_opt;

	// First call: initialize state, no events reported.
	if (!st->initialized) {
		for (int i = 0; i < 4; i++)
			st->prev[i] = now[i];
		st->initialized = 1;
		st->last_time = current_time;
		return 0;
	}

	size_t written = 0;

	for (int i = 0; i < 4; i++) {
		// Not GPIO pin now or before: ignore (Python: if None either side -> continue)
		if (now[i] < 0 || st->prev[i] < 0)
			continue;

		// No changes
		if (now[i] == st->prev[i])
			continue;

		mcp2221_gpio_event_type_t type = (st->prev[i] == 0 && now[i] == 1) ? MCP2221_GPIO_EVENT_RISE : MCP2221_GPIO_EVENT_FALL;

		if (!mask_allows(st->filter_mask, i, type))
			continue;

		if (written < max_events) {
			mcp2221_gpio_event_t *ev = &out_events[written];
			ev->gpio = (uint8_t)i;
			ev->type = type;
			ev->time = current_time;
			ev->last_time = st->last_time;
			snprintf(ev->id, sizeof(ev->id), "GPIO%d_%s", i, type == MCP2221_GPIO_EVENT_RISE ? "RISE" : "FALL");
			written++;
		}
	}

	// Update state like Python after scanning all pins
	for (int i = 0; i < 4; i++)
		st->prev[i] = now[i];
	st->last_time = current_time;

	return (int)written;
}
