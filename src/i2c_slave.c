#include "i2c_slave.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "exceptions.h"

#define MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES 4
#define MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES 256

static int is_valid_register_bytes(int bytes) {
	return bytes >= 1 && bytes <= MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES;
}

static int is_valid_transfer_length(size_t length) {
	return length > 0 && length <= MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES;
}

static int is_valid_slave(const I2C_Slave *slave) {
	return slave && slave->mcp && slave->addr <= MCP2221_I2C_ADDR_7BIT_MAX && is_valid_register_bytes(slave->reg_bytes);
}

// Helper: swap byte order
static void encode_register(uint32_t reg, int bytes, int little, uint8_t *out) {
	if (little) {
		for (int i = 0; i < bytes; ++i)
			out[i] = (reg >> (8 * i)) & 0xFF;
	} else {
		for (int i = 0; i < bytes; ++i)
			out[bytes - 1 - i] = (reg >> (8 * i)) & 0xFF;
	}
}

mcp_err_t mcp2221_i2c_slave_init(I2C_Slave *slave, MCP2221 *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz, int reg_bytes,
				   const char *reg_byteorder) {
	if (!slave || !mcp || addr > MCP2221_I2C_ADDR_7BIT_MAX)
		return MCP_ERR_INVALID;

	int rb = (reg_bytes <= 0) ? 1 : reg_bytes;
	if (!is_valid_register_bytes(rb))
		return MCP_ERR_INVALID;

	slave->mcp = mcp;
	slave->addr = addr;
	slave->reg_bytes = rb;

	if (!reg_byteorder || strcmp(reg_byteorder, "big") == 0)
		slave->reg_byteorder = 0;
	else
		slave->reg_byteorder = 1;

	// Set I2C speed
	mcp_err_t e = mcp2221_i2c_set_speed(mcp, i2c_speed_hz);
	if (e != MCP_ERR_OK)
		return e;

	// Test Device
	if (!force && !mcp2221_i2c_slave_is_present(slave))
		return MCP_ERR_NOT_ACK;

	return MCP_ERR_OK;
}

int mcp2221_i2c_slave_is_present(I2C_Slave *slave) {
	if (!is_valid_slave(slave))
		return 0;

	uint8_t tmp = 0;
	mcp_err_t e = mcp2221_i2c_read_ex(slave->mcp, slave->addr, &tmp, 1, MCP2221_I2C_KIND_NORMAL, 50);
	if (e == MCP_ERR_NOT_ACK)
		return 0;

	return (e == MCP_ERR_OK);
}

mcp_err_t mcp2221_i2c_slave_read_register(I2C_Slave *slave, uint32_t reg, uint8_t *buffer, size_t length, int reg_bytes,
							const char *reg_byteorder) {
	if (!is_valid_slave(slave) || !buffer || !is_valid_transfer_length(length))
		return MCP_ERR_INVALID;

	int rb = reg_bytes > 0 ? reg_bytes : slave->reg_bytes;
	if (!is_valid_register_bytes(rb))
		return MCP_ERR_INVALID;

	int little =
		(reg_byteorder && strcmp(reg_byteorder, "little") == 0) || ((!reg_byteorder) && slave->reg_byteorder == 1);

	uint8_t regbuf[4];
	encode_register(reg, rb, little, regbuf);

	// Write register without stop, then read with repeated start.
	mcp_err_t e = mcp2221_i2c_write_ex(slave->mcp, slave->addr, regbuf, rb, MCP2221_I2C_KIND_NO_STOP, 50);
	if (e)
		return e;

	// read (restart)
	return mcp2221_i2c_read_ex(slave->mcp, slave->addr, buffer, length, MCP2221_I2C_KIND_REPEATED_START, 50);
}

mcp_err_t mcp2221_i2c_slave_read(I2C_Slave *slave, uint8_t *buffer, size_t length) {
	if (!is_valid_slave(slave) || !buffer || !is_valid_transfer_length(length))
		return MCP_ERR_INVALID;

	return mcp2221_i2c_read_ex(slave->mcp, slave->addr, buffer, length, MCP2221_I2C_KIND_NORMAL, 50);
}

mcp_err_t mcp2221_i2c_slave_write_register(I2C_Slave *slave, uint32_t reg, const uint8_t *data, size_t length, int reg_bytes,
							 const char *reg_byteorder) {
	if (!is_valid_slave(slave) || length > MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES || (length > 0 && !data))
		return MCP_ERR_INVALID;

	int rb = reg_bytes > 0 ? reg_bytes : slave->reg_bytes;
	if (!is_valid_register_bytes(rb))
		return MCP_ERR_INVALID;

	int little =
		(reg_byteorder && strcmp(reg_byteorder, "little") == 0) || ((!reg_byteorder) && slave->reg_byteorder == 1);

	uint8_t tmp[MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES + MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES];
	encode_register(reg, rb, little, tmp);
	if (length > 0)
		memcpy(tmp + rb, data, length);

	// normal write
	return mcp2221_i2c_write_ex(slave->mcp, slave->addr, tmp, rb + length, MCP2221_I2C_KIND_NORMAL, 50);
}

mcp_err_t mcp2221_i2c_slave_write(I2C_Slave *slave, const uint8_t *data, size_t length) {
	if (!is_valid_slave(slave) || !data || !is_valid_transfer_length(length))
		return MCP_ERR_INVALID;

	return mcp2221_i2c_write_ex(slave->mcp, slave->addr, data, length, MCP2221_I2C_KIND_NORMAL, 50);
}


mcp_err_t i2c_slave_init(I2C_Slave *slave, MCP2221 *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz, int reg_bytes,
				   const char *reg_byteorder) {
	return mcp2221_i2c_slave_init(slave, mcp, addr, force, i2c_speed_hz, reg_bytes, reg_byteorder);
}

int i2c_slave_is_present(I2C_Slave *slave) {
	return mcp2221_i2c_slave_is_present(slave);
}

mcp_err_t i2c_slave_read_register(I2C_Slave *slave, uint32_t reg, uint8_t *buffer, size_t length, int reg_bytes,
							const char *reg_byteorder) {
	return mcp2221_i2c_slave_read_register(slave, reg, buffer, length, reg_bytes, reg_byteorder);
}

mcp_err_t i2c_slave_read(I2C_Slave *slave, uint8_t *buffer, size_t length) {
	return mcp2221_i2c_slave_read(slave, buffer, length);
}

mcp_err_t i2c_slave_write_register(I2C_Slave *slave, uint32_t reg, const uint8_t *data, size_t length, int reg_bytes,
							 const char *reg_byteorder) {
	return mcp2221_i2c_slave_write_register(slave, reg, data, length, reg_bytes, reg_byteorder);
}

mcp_err_t i2c_slave_write(I2C_Slave *slave, const uint8_t *data, size_t length) {
	return mcp2221_i2c_slave_write(slave, data, length);
}
