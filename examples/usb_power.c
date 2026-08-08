#include <stdio.h>

#include "mcp2221.h"
#include "mcp2221_constants.h"
#include "mcp2221_flash_info.h"
#include "mcp2221_usb.h"

int main(void) {
	mcp2221_t *dev = mcp2221_open_simple(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		100000);

	if (!dev) {
		fprintf(stderr, "Failed to open MCP2221.\n");
		return 1;
	}

	mcp2221_error_code_t err;

	/*
	 * Advertise USB Remote Wake-up capability.
	 *
	 * This does not wake the host by itself. Remote Wake-up must also be
	 * allowed by the operating system and an appropriate wake-up source
	 * (for example GP1/IOC) must be configured.
	 */
	err = mcp2221_usb_set_remote_wakeup(dev, 1);
	if (err != MCP2221_ERR_OK)
		goto fail;

	/*
	 * This example assumes a normal USB bus-powered device.
	 *
	 * Do not set this to 1 unless the actual hardware is self-powered.
	 */
	err = mcp2221_usb_set_self_powered(dev, 0);
	if (err != MCP2221_ERR_OK)
		goto fail;

	/*
	 * Advertise a maximum USB bus current of 100 mA.
	 * The MCP2221 stores this internally in units of 2 mA.
	 */
	err = mcp2221_usb_set_requested_current(dev, 100);
	if (err != MCP2221_ERR_OK)
		goto fail;

	/*
	 * Persist the staged USB configuration.
	 */
	err = mcp2221_flash_save_config(dev);
	if (err != MCP2221_ERR_OK)
		goto fail;

	printf("USB power configuration saved:\n");
	printf("  Remote Wake-up: enabled\n");
	printf("  Self-powered:   no\n");
	printf("  Bus current:    100 mA\n");
	printf("\n");
	printf("Reset or reconnect the MCP2221 for the new USB attributes "
	       "to take effect.\n");

	mcp2221_close(dev);
	return 0;

fail:
	fprintf(stderr, "USB power configuration failed (error %d).\n", err);
	mcp2221_close(dev);
	return 1;
}