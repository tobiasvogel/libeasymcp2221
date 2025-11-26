#include "i2c_slave.h"

#include <stdlib.h>
#include <string.h>

#include "exceptions.h"

// Helper-function: swap byteorder
static void encode_register(uint32_t reg, int bytes, int little, uint8_t *out) {
	if (little) {
		for (int i = 0; i < bytes; ++i)
			out[i] = (reg >> (8 * i)) & 0xFF;
	} else {
		for (int i = 0; i < bytes; ++i)
			out[bytes - 1 - i] = (reg >> (8 * i)) & 0xFF;
	}
}

mcp_err_t i2c_slave_init(I2C_Slave *slave, MCP2221 *mcp, uint8_t addr, int force, uint32_t speed_hz, int reg_bytes,
				   const char *reg_byteorder) {
	if (!slave || !mcp)
		return MCP_ERR_INVALID;

	slave->mcp = mcp;
	slave->addr = addr;
	slave->reg_bytes = (reg_bytes <= 0) ? 1 : reg_bytes;

	if (!reg_byteorder || strcmp(reg_byteorder, "big") == 0)
		slave->reg_byteorder = 0;
	else
		slave->reg_byteorder = 1;

	// Set I2C speed
	mcp_err_t e = mcp2221_i2c_speed(mcp, speed_hz);
	if (e != MCP_ERR_OK)
		return e;

	// Test Device
	if (!force && !i2c_slave_is_present(slave))
		return MCP_ERR_NOT_ACK;

	return MCP_ERR_OK;
}

int i2c_slave_is_present(I2C_Slave *slave) {
	uint8_t tmp = 0;
	mcp_err_t e = mcp2221_i2c_read(slave->mcp, slave->addr, &tmp, 1, 0 /* normal read */, 50);
	if (e == MCP_ERR_NOT_ACK)
		return 0;

	return (e == MCP_ERR_OK);
}

int i2c_slave_read_register(I2C_Slave *slave, uint32_t reg, uint8_t *buffer, size_t length, int reg_bytes,
							const char *reg_byteorder) {
	int rb = reg_bytes > 0 ? reg_bytes : slave->reg_bytes;

	int little =
		(reg_byteorder && strcmp(reg_byteorder, "little") == 0) || ((!reg_byteorder) && slave->reg_byteorder == 1);

	uint8_t regbuf[4];
	encode_register(reg, rb, little, regbuf);

	// Write Register -> nonstop
	mcp_err_t e = mcp2221_i2c_write(slave->mcp, slave->addr, regbuf, rb, 2 /* nonstop */, 50);
	if (e)
		return e;

	// read (restart)
	return mcp2221_i2c_read(slave->mcp, slave->addr, buffer, length, 1 /* restart */, 50);
}

int i2c_slave_read(I2C_Slave *slave, uint8_t *buffer, size_t length) {
	return mcp2221_i2c_read(slave->mcp, slave->addr, buffer, length, 0 /* normal */, 50);
}

int i2c_slave_write_register(I2C_Slave *slave, uint32_t reg, const uint8_t *data, size_t length, int reg_bytes,
							 const char *reg_byteorder) {
	int rb = reg_bytes > 0 ? reg_bytes : slave->reg_bytes;

	int little =
		(reg_byteorder && strcmp(reg_byteorder, "little") == 0) || ((!reg_byteorder) && slave->reg_byteorder == 1);

	uint8_t tmp[4 + 256];
	encode_register(reg, rb, little, tmp);
	memcpy(tmp + rb, data, length);

	// normal write
	return mcp2221_i2c_write(slave->mcp, slave->addr, tmp, rb + length, 0 /* normal */, 50);
}

int i2c_slave_write(I2C_Slave *slave, const uint8_t *data, size_t length) {
	return mcp2221_i2c_write(slave->mcp, slave->addr, data, length, 0 /* normal */, 50);
}
