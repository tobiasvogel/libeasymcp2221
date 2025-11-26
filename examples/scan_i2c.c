#include <stdint.h>
#include <stdio.h>

#include "i2c_slave.h"
#include "mcp2221.h"

int main(void) {
	MCP2221 *dev = mcp2221_open(0x04D8, 0x00DD,
								0,	   // first device (index 0)
								NULL,  // no serial filter
								500,   // read timeout ms
								1,	   // retries
								0,	   // debug messages off
								0	   // trace packets off
	);

	if (!dev) {
		fprintf(stderr, "MCP2221 not found.\n");
		return 1;
	}

	printf("Scanning I2C bus using MCP2221 at 100kHz...\n");
	mcp2221_i2c_speed(dev, 100000);

	printf("    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	printf("00: ");

	for (uint8_t addr = 0; addr < 128; addr++) {
		if (addr % 16 == 0 && addr != 0)
			printf("\n%02X: ", addr);

		/*
		   Check presence:
		   Create a temporary I2C_Slave object and do a 1-byte read.
		   If ACK -> device present.
		*/
		I2C_Slave tmp;
		int r = mcp2221_create_i2c_slave(dev, &tmp, addr, 1, 100000, 1, "big");

		if (r != MCP_ERR_OK) {
			printf("-- ");
			continue;
		}

		uint8_t data;
		r = i2c_slave_read(&tmp, &data, 1);

		if (r == MCP_ERR_OK)
			printf("%02X ", addr);
		else
			printf("-- ");
	}

	printf("\nDone.\n");
	mcp2221_close(dev);
	return 0;
}
