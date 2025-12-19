#include "mcp2221_gpio_poll.h"

#include <string.h>
#include <time.h>
#include <stdio.h>

#include "constants.h"

#define GPIO_ERROR 0xEE

static double wall_time_seconds(void) {
#if defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
#else
	return (double)time(NULL);
#endif
}

void mcp2221_gpio_poll_init(MCP_GPIO_PollState *st) {
	for (int i = 0; i < 4; i++)
		st->prev[i] = -2; /* -2 = uninitialized special value */
	st->initialized = 0;
	st->last_time = 0.0;
	st->filter_mask = 0; /* 0 = accept all, like Python's default [] */
}

void mcp2221_gpio_poll_set_filter_mask(MCP_GPIO_PollState *st, uint16_t mask) {
	if (!st)
		return;
	st->filter_mask = mask;
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

static int mask_allows(uint16_t mask, int pin, MCP_GPIO_EventType type) {
	if (mask == 0)
		return 1; /* 0 = all events, mirroring Python filter=[] */
	int bit = pin * 2 + (type == MCP_GPIO_EVENT_FALL ? 1 : 0);
	return (mask & (1u << bit)) != 0;
}

int mcp2221_gpio_poll_events(MCP2221 *dev, MCP_GPIO_PollState *st, const uint16_t *filter_mask_opt,
							MCP_GPIO_Event *out_events, size_t max_events) {
	if (!dev || !st || (!out_events && max_events > 0))
		return MCP_ERR_INVALID;

	uint8_t cmd = CMD_GET_GPIO_VALUES;
	uint8_t resp[PACKET_SIZE];

	mcp_err_t err = mcp2221_send_cmd(dev, &cmd, 1, resp);
	if (err)
		return err;

	int now[4];
	now[0] = (resp[2] == GPIO_ERROR) ? -1 : resp[2];
	now[1] = (resp[4] == GPIO_ERROR) ? -1 : resp[4];
	now[2] = (resp[6] == GPIO_ERROR) ? -1 : resp[6];
	now[3] = (resp[8] == GPIO_ERROR) ? -1 : resp[8];

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

		MCP_GPIO_EventType type = (st->prev[i] == 0 && now[i] == 1) ? MCP_GPIO_EVENT_RISE : MCP_GPIO_EVENT_FALL;

		if (!mask_allows(st->filter_mask, i, type))
			continue;

		if (written < max_events) {
			MCP_GPIO_Event *ev = &out_events[written];
			ev->gpio = (uint8_t)i;
			ev->type = type;
			ev->time = current_time;
			ev->last_time = st->last_time;
			snprintf(ev->id, sizeof(ev->id), "GPIO%d_%s", i, type == MCP_GPIO_EVENT_RISE ? "RISE" : "FALL");
			written++;
		}
	}

	// Update state like Python after scanning all pins
	for (int i = 0; i < 4; i++)
		st->prev[i] = now[i];
	st->last_time = current_time;

	return (int)written;
}
