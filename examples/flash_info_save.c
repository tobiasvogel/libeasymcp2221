#include <stdio.h>

#include "mcp2221.h"
#include "mcp2221_flash_info.h"

int main(void) {
	MCP2221 *dev = mcp2221_open_simple(0x04D8, 0x00DD, 0, NULL, 100000);
	if (!dev) {
		fprintf(stderr, "Failed to open MCP2221\n");
		return 1;
	}

	MCP2221_FlashInfo info;
	int err = mcp2221_flash_read_info(dev, &info);
	if (err != MCP_ERR_OK) {
		fprintf(stderr, "flash_read_info failed: %d\n", err);
		mcp2221_close(dev);
		return 1;
	}

	printf("USB Manufacturer: %s\n", info.usb_manufacturer_str);
	printf("USB Product     : %s\n", info.usb_product_str);
	printf("USB Serial      : %s\n", info.usb_serial_str);
	printf("Factory Serial  : %s\n", info.usb_factory_serial_str);

	printf("\nWriting current SRAM settings back to flash (like Python save_config). This is persistent.\n");
	err = mcp2221_flash_save_config(dev);
	if (err != MCP_ERR_OK) {
		fprintf(stderr, "flash_save_config failed: %d\n", err);
		mcp2221_close(dev);
		return 1;
	}
	printf("Done.\n");

	mcp2221_close(dev);
	return 0;
}
