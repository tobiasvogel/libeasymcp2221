#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(unsigned ms) { Sleep(ms); }
#else
#include <time.h>
static void sleep_ms(unsigned ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}
#endif

#include "constants.h"
#include "mcp2221.h"
#include "mcp2221_gpio_poll.h"
#include "mcp2221_pin.h"

/*
 * Example: wait for a specific GPIO event, mirroring EasyMCP2221's Python example:
 *
 *   >>> while not mcp.GPIO_poll(["GPIO0_RISE"]):
 *   ...    pass
 *
 * In C, we use `mcp2221_gpio_poll_events()` plus a filter mask.
 */

int main(void) {
	MCP2221 *dev =
		mcp2221_open(DEV_DEFAULT_VID, DEV_DEFAULT_PID, 0, NULL, 500, 3, 0, 0);
	if (!dev) {
		fprintf(stderr, "Failed to open MCP2221.\n");
		return 1;
	}

	// Ensure GP0 is configured as GPIO input (Python: set_pin_function(gp0="GPIO_IN"))
	if (mcp2221_set_pin_function(dev, MCP_GP0, MCP_PIN_FUNC_GPIO_IN) != MCP_ERR_OK) {
		fprintf(stderr, "Failed to configure GP0 as GPIO input.\n");
		mcp2221_close(dev);
		return 2;
	}

	MCP_GPIO_PollState st;
	mcp2221_gpio_poll_init(&st);

	// Filter: only accept GPIO0_RISE
	uint16_t filter = MCP_GPIO_POLL_MASK_RISE(0);

	printf("Waiting for GPIO0_RISE...\n");

	while (1) {
		MCP_GPIO_Event events[8];
		int n = mcp2221_gpio_poll_events(dev, &st, &filter, events, 8);
		if (n < 0) {
			fprintf(stderr, "GPIO poll failed: %d\n", n);
			mcp2221_close(dev);
			return 3;
		}

		if (n > 0) {
			// Filter is set to only GPIO0_RISE, so the first event ends the wait.
			printf("%s at %f (last_time=%f)\n", events[0].id, events[0].time, events[0].last_time);
			break;
		}

		// Avoid spinning at 100% CPU (Python example is a busy loop; this is friendlier).
		sleep_ms(1);
	}

	mcp2221_close(dev);
	return 0;
}

