#include <stddef.h>
#include <stdint.h>
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

	/*
	 * Example target: a 24LC256-compatible EEPROM at address 0x50.
	 * It uses a two-byte, big-endian register/address field.
	 */
	mcp2221_i2c_slave_t eeprom;
	err = mcp2221_i2c_slave_init(&eeprom, dev, 0x50, 0, 100000, 2,
	                             MCP2221_I2C_BYTE_ORDER_BIG);
	if (err != MCP2221_ERR_OK) {
		fprintf(stderr, "mcp2221_i2c_slave_init failed: %s\n",
		        mcp2221_error_code_to_string(err));
		mcp2221_close(dev);
		return 1;
	}

	uint8_t data[16];
	err = mcp2221_i2c_slave_read_register(
		&eeprom, 0x0000, data, sizeof(data), 0,
		MCP2221_I2C_BYTE_ORDER_DEFAULT);
	if (err != MCP2221_ERR_OK) {
		fprintf(stderr, "mcp2221_i2c_slave_read_register failed: %s\n",
		        mcp2221_error_code_to_string(err));
		mcp2221_close(dev);
		return 1;
	}

	printf("EEPROM 0x50, bytes at register 0x0000:");
	for (size_t i = 0; i < sizeof(data); ++i)
		printf(" %02x", data[i]);
	putchar('\n');

	mcp2221_close(dev);
	return 0;
}
