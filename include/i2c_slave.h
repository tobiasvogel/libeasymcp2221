#ifndef MCP2221_I2C_SLAVE_H
#define MCP2221_I2C_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

/* Caller-owned I2C slave context.
 *
 * This is a public value type, not an opaque handle. Applications may allocate
 * it statically, on the stack or as part of another structure.
 */
struct mcp2221_i2c_slave {
	mcp2221_t *mcp;
	uint8_t addr;
	int reg_bytes;
	int reg_byteorder; /* 0 = big endian, 1 = little endian */
};

/* Error-returning I2C slave functions return MCP2221_ERR_OK on success or
 * another mcp2221_error_code_t value on error.
 *
 * mcp2221_i2c_slave_check_present() preserves the Python helper semantics:
 * an address NACK is reported as MCP2221_ERR_OK with *is_present set to 0,
 * while transport, timeout and other I2C errors are returned to the caller.
 *
 * mcp2221_i2c_slave_is_present() is a boolean-style compatibility helper
 * which returns 1 only when the device ACKs. It cannot distinguish an address
 * NACK from another error.
 */

/* Preferred mcp2221_* names. */
mcp2221_error_code_t mcp2221_i2c_slave_init(mcp2221_i2c_slave_t *slave, mcp2221_t *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz,
						   int reg_bytes, const char *reg_byteorder);
mcp2221_error_code_t mcp2221_i2c_slave_check_present(mcp2221_i2c_slave_t *slave, int *is_present);
int mcp2221_i2c_slave_is_present(mcp2221_i2c_slave_t *slave);
mcp2221_error_code_t mcp2221_i2c_slave_read_register(mcp2221_i2c_slave_t *slave, uint32_t reg, uint8_t *buffer, size_t length,
									int reg_bytes, const char *reg_byteorder);
mcp2221_error_code_t mcp2221_i2c_slave_read(mcp2221_i2c_slave_t *slave, uint8_t *buffer, size_t length);
mcp2221_error_code_t mcp2221_i2c_slave_write_register(mcp2221_i2c_slave_t *slave, uint32_t reg, const uint8_t *data, size_t length,
									 int reg_bytes, const char *reg_byteorder);
mcp2221_error_code_t mcp2221_i2c_slave_write(mcp2221_i2c_slave_t *slave, const uint8_t *data, size_t length);

#endif	// MCP2221_I2C_SLAVE_H
