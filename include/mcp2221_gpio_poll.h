#ifndef MCP2221_GPIO_POLL_H
#define MCP2221_GPIO_POLL_H

#include <stdint.h>
#include <stddef.h>

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
	double last_time;	   /* wall-clock time (seconds) of last poll */
	uint16_t filter_mask; /* 0 = all, else bitmask of allowed events */
} MCP_GPIO_PollState;

typedef enum {
	MCP_GPIO_EVENT_RISE = 0,
	MCP_GPIO_EVENT_FALL = 1,
} MCP_GPIO_EventType;

typedef struct {
	char id[12]; /* "GPIO3_FALL" + '\0' */
	uint8_t gpio; /* 0..3 */
	MCP_GPIO_EventType type;
	double time;	  /* current wall-clock time (seconds) */
	double last_time; /* previous wall-clock time (seconds) */
} MCP_GPIO_Event;

// Filter mask bits: bit 0 = GPIO0_RISE, bit 1 = GPIO0_FALL, bit 2 = GPIO1_RISE, ...
#define MCP_GPIO_POLL_MASK_RISE(pin) (1u << ((pin) * 2))
#define MCP_GPIO_POLL_MASK_FALL(pin) (1u << ((pin) * 2 + 1))

// Initialize
void mcp2221_gpio_poll_init(MCP_GPIO_PollState *st);

// Set filter mask (0 = all events). Mirrors Python's persistent filter behavior.
void mcp2221_gpio_poll_set_filter_mask(MCP_GPIO_PollState *st, uint16_t mask);

// Poll and notify on change
int mcp2221_gpio_poll(MCP2221 *dev, MCP_GPIO_PollState *st, MCP_GPIO_Change out[4]);

/**
 * Poll GPIO changes and return a list of events, mirroring EasyMCP2221's `GPIO_poll()` semantics.
 *
 * - First call returns 0 events and initializes internal state.
 * - Events are emitted only for pins that are GPIO (i.e. not -1).
 * - Filter behavior:
 *     - If `filter_mask_opt` is NULL, the last filter is preserved.
 *     - If `*filter_mask_opt` is 0, all events are returned.
 *     - Otherwise only events whose bit is set are returned.
 *
 * Returns: number of events written to `out_events` (0..max_events), or <0 on error.
 */
int mcp2221_gpio_poll_events(MCP2221 *dev, MCP_GPIO_PollState *st, const uint16_t *filter_mask_opt,
							MCP_GPIO_Event *out_events, size_t max_events);

#endif	// MCP2221_GPIO_POLL_H
