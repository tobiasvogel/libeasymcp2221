#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
#include "i2c_slave.h"
#include "mcp2221.h"

int main(void) {
	mcp2221_t *dev = mcp2221_open(MCP2221_DEV_DEFAULT_VID, MCP2221_DEV_DEFAULT_PID,
								0,	   // first device (index 0)
								NULL,  // no serial filter
								500,   // read timeout ms
								1,	   // retries
								0,	   // debug messages off
								0	   // trace packets off
	);

	if (!dev) {
		fprintf(stderr, "MCP2221 not found.\n");
		return EXIT_FAILURE;
	}

	printf("Scanning I2C bus using MCP2221 at 100kHz...\n");
	mcp2221_error_code_t err = mcp2221_i2c_set_speed(dev, 100000);
	if (err != MCP2221_ERR_OK) {
		fprintf(stderr, "Failed to set I2C speed: %s\n",
				mcp2221_error_code_to_string(err));
		mcp2221_close(dev);
		return EXIT_FAILURE;
	}

	printf("    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	printf("00: ");

	for (uint8_t addr = 0; addr < 128; addr++) {
		if (addr % 16 == 0 && addr != 0)
			printf("\n%02X: ", addr);

		/*
		   Check presence:
		   Create a temporary mcp2221_i2c_slave_t object and do a 1-byte read.
		   If ACK -> device present.
		*/
		mcp2221_i2c_slave_t tmp;
		err = mcp2221_i2c_slave_create(dev, &tmp, addr, 1, 100000, 1, "big");

		if (err != MCP2221_ERR_OK) {
			printf("-- ");
			continue;
		}

		uint8_t data;
		err = mcp2221_i2c_slave_read(&tmp, &data, 1);

		if (err == MCP2221_ERR_OK)
			printf("%02X ", addr);
		else
			printf("-- ");
	}

	printf("\nDone.\n");
	mcp2221_close(dev);
	return EXIT_SUCCESS;
}
