#include <stdio.h>

#include "libeasymcp2221.h"

int main(void) {
	mcp2221_t *dev = NULL;
	mcp2221_error_code_t err =
		mcp2221_open_simple(MCP2221_DEV_DEFAULT_VID,
		                    MCP2221_DEV_DEFAULT_PID,
		                    0, NULL, 100000, &dev);
	if (err != MCP2221_ERR_OK) {
		fprintf(stderr, "Failed to open MCP2221: %s\n",
		        mcp2221_error_code_to_string(err));
		return 1;
	}

	mcp2221_revision_t revision;
	err = mcp2221_revision(dev, &revision);
	if (err != MCP2221_ERR_OK) {
		fprintf(stderr, "mcp2221_revision failed: %s\n",
		        mcp2221_error_code_to_string(err));
		mcp2221_close(dev);
		return 1;
	}

	printf("Hardware revision: %u.%u\n",
	       (unsigned)revision.hardware_major,
	       (unsigned)revision.hardware_minor);
	printf("Firmware revision: %u.%u\n",
	       (unsigned)revision.firmware_major,
	       (unsigned)revision.firmware_minor);

	mcp2221_flash_info_t info;
	err = mcp2221_flash_read_info(dev, &info);
	if (err != MCP2221_ERR_OK) {
		fprintf(stderr, "mcp2221_flash_read_info failed: %s\n",
		        mcp2221_error_code_to_string(err));
		mcp2221_close(dev);
		return 1;
	}

	printf("USB manufacturer: %s\n", info.usb_manufacturer_str);
	printf("USB product     : %s\n", info.usb_product_str);
	printf("USB serial      : %s\n", info.usb_serial_str);
	printf("Factory serial  : %s\n", info.usb_factory_serial_str);

	mcp2221_close(dev);
	return 0;
}
