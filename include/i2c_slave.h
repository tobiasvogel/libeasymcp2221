#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

struct I2C_Slave {
	MCP2221 *mcp;
	uint8_t addr;
	int reg_bytes;
	int reg_byteorder; /* 0 = big endian, 1 = little endian */
};

typedef struct I2C_Slave I2C_Slave;

// Initializer
int i2c_slave_init(I2C_Slave *slave, MCP2221 *mcp, uint8_t addr, int force, uint32_t speed_hz, int reg_bytes,
				   const char *reg_byteorder);

// Check if device is present
int i2c_slave_is_present(I2C_Slave *slave);

// Read register
int i2c_slave_read_register(I2C_Slave *slave, uint32_t reg, uint8_t *buffer, size_t length, int reg_bytes,
							const char *reg_byteorder);

// Read
int i2c_slave_read(I2C_Slave *slave, uint8_t *buffer, size_t length);

// Write register
int i2c_slave_write_register(I2C_Slave *slave, uint32_t reg, const uint8_t *data, size_t length, int reg_bytes,
							 const char *reg_byteorder);

// Write
int i2c_slave_write(I2C_Slave *slave, const uint8_t *data, size_t length);

#endif	// I2C_SLAVE_H
