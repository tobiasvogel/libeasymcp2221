#ifndef MCP2221_I2C_SLAVE_H
#define MCP2221_I2C_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

struct mcp2221_i2c_slave {
	mcp2221_t *mcp;
	uint8_t addr;
	int reg_bytes;
	int reg_byteorder; /* 0 = big endian, 1 = little endian */
};

/* Error-returning I2C slave functions return MCP2221_ERR_OK on success or
 * another mcp2221_error_code_t value on error.
 * mcp2221_i2c_slave_is_present() is a boolean-style probe
 * and returns 1 when the device ACKs, otherwise 0.
 */

/* Preferred mcp2221_* names. */
mcp2221_error_code_t mcp2221_i2c_slave_init(mcp2221_i2c_slave_t *slave, mcp2221_t *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz,
						   int reg_bytes, const char *reg_byteorder);
int mcp2221_i2c_slave_is_present(mcp2221_i2c_slave_t *slave);
mcp2221_error_code_t mcp2221_i2c_slave_read_register(mcp2221_i2c_slave_t *slave, uint32_t reg, uint8_t *buffer, size_t length,
									int reg_bytes, const char *reg_byteorder);
mcp2221_error_code_t mcp2221_i2c_slave_read(mcp2221_i2c_slave_t *slave, uint8_t *buffer, size_t length);
mcp2221_error_code_t mcp2221_i2c_slave_write_register(mcp2221_i2c_slave_t *slave, uint32_t reg, const uint8_t *data, size_t length,
									 int reg_bytes, const char *reg_byteorder);
mcp2221_error_code_t mcp2221_i2c_slave_write(mcp2221_i2c_slave_t *slave, const uint8_t *data, size_t length);

/* Legacy aliases; scheduled for removal in a future major version. */
MCP2221_DEPRECATED("use mcp2221_i2c_slave_init") mcp_err_t i2c_slave_init(I2C_Slave *slave, MCP2221 *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz, int reg_bytes,
				   const char *reg_byteorder);
MCP2221_DEPRECATED("use mcp2221_i2c_slave_is_present") int i2c_slave_is_present(I2C_Slave *slave);
MCP2221_DEPRECATED("use mcp2221_i2c_slave_read_register") mcp_err_t i2c_slave_read_register(I2C_Slave *slave, uint32_t reg, uint8_t *buffer, size_t length, int reg_bytes,
							const char *reg_byteorder);
MCP2221_DEPRECATED("use mcp2221_i2c_slave_read") mcp_err_t i2c_slave_read(I2C_Slave *slave, uint8_t *buffer, size_t length);
MCP2221_DEPRECATED("use mcp2221_i2c_slave_write_register") mcp_err_t i2c_slave_write_register(I2C_Slave *slave, uint32_t reg, const uint8_t *data, size_t length, int reg_bytes,
							 const char *reg_byteorder);
MCP2221_DEPRECATED("use mcp2221_i2c_slave_write") mcp_err_t i2c_slave_write(I2C_Slave *slave, const uint8_t *data, size_t length);

#endif	// MCP2221_I2C_SLAVE_H
