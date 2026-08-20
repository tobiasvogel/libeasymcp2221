#include "mcp2221_i2c_slave.h"

#include <stdlib.h>
#include <string.h>

#include "mcp2221_internal_constants.h"

#define MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES 4
#define MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES 256

static int is_valid_register_bytes(int bytes) {
	return bytes >= 1 && bytes <= MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES;
}

static int is_valid_register_value(uint32_t reg, int bytes) {
	if (!is_valid_register_bytes(bytes))
		return 0;
	if (bytes == MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES)
		return 1;
	return reg < (1u << (8 * bytes));
}

static int is_valid_transfer_length(size_t length) {
	return length > 0 && length <= MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES;
}

static int is_valid_byte_order(mcp2221_i2c_byte_order_t byte_order) {
	return byte_order == MCP2221_I2C_BYTE_ORDER_BIG || byte_order == MCP2221_I2C_BYTE_ORDER_LITTLE;
}

static int is_valid_slave(const mcp2221_i2c_slave_t *slave) {
	return slave && slave->mcp && slave->addr <= MCP2221_I2C_ADDR_7BIT_MAX && is_valid_register_bytes(slave->reg_bytes) &&
		   is_valid_byte_order(slave->reg_byteorder);
}

static mcp2221_error_code_t resolve_byte_order(const mcp2221_i2c_slave_t *slave, mcp2221_i2c_byte_order_t requested,
						      mcp2221_i2c_byte_order_t *resolved) {
	if (!slave || !resolved)
		return MCP2221_ERR_INVALID;

	if (requested == MCP2221_I2C_BYTE_ORDER_DEFAULT)
		requested = slave->reg_byteorder;

	if (!is_valid_byte_order(requested))
		return MCP2221_ERR_INVALID;

	*resolved = requested;
	return MCP2221_ERR_OK;
}

// Helper: encode register address using the selected byte order.
static void encode_register(uint32_t reg, int bytes, mcp2221_i2c_byte_order_t byte_order, uint8_t *out) {
	if (byte_order == MCP2221_I2C_BYTE_ORDER_LITTLE) {
		for (int i = 0; i < bytes; ++i)
			out[i] = (reg >> (8 * i)) & 0xFF;
	} else {
		for (int i = 0; i < bytes; ++i)
			out[bytes - 1 - i] = (reg >> (8 * i)) & 0xFF;
	}
}

mcp2221_error_code_t mcp2221_i2c_slave_init(mcp2221_i2c_slave_t *slave, mcp2221_t *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz, int reg_bytes,
				   mcp2221_i2c_byte_order_t reg_byteorder) {
	if (!slave)
		return MCP2221_ERR_INVALID;

	/* A failed initialization must never leave a usable-looking context. */
	memset(slave, 0, sizeof(*slave));

	if (!mcp || addr > MCP2221_I2C_ADDR_7BIT_MAX)
		return MCP2221_ERR_INVALID;

	int rb = (reg_bytes <= 0) ? 1 : reg_bytes;
	if (!is_valid_register_bytes(rb))
		return MCP2221_ERR_INVALID;

	if (reg_byteorder == MCP2221_I2C_BYTE_ORDER_DEFAULT)
		reg_byteorder = MCP2221_I2C_BYTE_ORDER_BIG;
	if (!is_valid_byte_order(reg_byteorder))
		return MCP2221_ERR_INVALID;

	mcp2221_i2c_slave_t tmp = {
		.mcp = mcp,
		.addr = addr,
		.reg_bytes = rb,
		.reg_byteorder = reg_byteorder
	};

	// Set I2C speed
	mcp2221_error_code_t err = mcp2221_i2c_set_speed(mcp, i2c_speed_hz);
	if (err != MCP2221_ERR_OK)
		return err;

	// Test Device
	if (!force) {
		int is_present = 0;
		err = mcp2221_i2c_slave_check_present(&tmp, &is_present);
		if (err != MCP2221_ERR_OK)
			return err;
		if (!is_present)
			return MCP2221_ERR_NOT_ACK;
	}

	*slave = tmp;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_i2c_slave_check_present(mcp2221_i2c_slave_t *slave, int *is_present) {
	if (!is_valid_slave(slave) || !is_present)
		return MCP2221_ERR_INVALID;

	uint8_t tmp = 0;
	mcp2221_error_code_t err = mcp2221_i2c_read_ex(slave->mcp, slave->addr, &tmp, 1, MCP2221_I2C_KIND_NORMAL, 50);

	if (err == MCP2221_ERR_NOT_ACK) {
		*is_present = 0;
		return MCP2221_ERR_OK;
	}

	if (err != MCP2221_ERR_OK)
		return err;

	*is_present = 1;
	return MCP2221_ERR_OK;
}

int mcp2221_i2c_slave_is_present(mcp2221_i2c_slave_t *slave) {
	int is_present = 0;

	if (mcp2221_i2c_slave_check_present(slave, &is_present) != MCP2221_ERR_OK)
		return 0;

	return is_present;
}

mcp2221_error_code_t mcp2221_i2c_slave_read_register(mcp2221_i2c_slave_t *slave, uint32_t reg, uint8_t *buffer, size_t length, int reg_bytes,
							mcp2221_i2c_byte_order_t reg_byteorder) {
	if (!is_valid_slave(slave) || !buffer || !is_valid_transfer_length(length))
		return MCP2221_ERR_INVALID;

	int rb = reg_bytes > 0 ? reg_bytes : slave->reg_bytes;
	if (!is_valid_register_value(reg, rb))
		return MCP2221_ERR_INVALID;

	mcp2221_i2c_byte_order_t byte_order;
	mcp2221_error_code_t err = resolve_byte_order(slave, reg_byteorder, &byte_order);
	if (err != MCP2221_ERR_OK)
		return err;

	uint8_t regbuf[4];
	encode_register(reg, rb, byte_order, regbuf);

	// Write register without stop, then read with repeated start.
	err = mcp2221_i2c_write_ex(slave->mcp, slave->addr, regbuf, rb, MCP2221_I2C_KIND_NO_STOP, 50);
	if (err)
		return err;

	// read (restart)
	return mcp2221_i2c_read_ex(slave->mcp, slave->addr, buffer, length, MCP2221_I2C_KIND_REPEATED_START, 50);
}

mcp2221_error_code_t mcp2221_i2c_slave_read(mcp2221_i2c_slave_t *slave, uint8_t *buffer, size_t length) {
	if (!is_valid_slave(slave) || !buffer || !is_valid_transfer_length(length))
		return MCP2221_ERR_INVALID;

	return mcp2221_i2c_read_ex(slave->mcp, slave->addr, buffer, length, MCP2221_I2C_KIND_NORMAL, 50);
}

mcp2221_error_code_t mcp2221_i2c_slave_write_register(mcp2221_i2c_slave_t *slave, uint32_t reg, const uint8_t *data, size_t length, int reg_bytes,
							 mcp2221_i2c_byte_order_t reg_byteorder) {
	if (!is_valid_slave(slave) || length > MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES || (length > 0 && !data))
		return MCP2221_ERR_INVALID;

	int rb = reg_bytes > 0 ? reg_bytes : slave->reg_bytes;
	if (!is_valid_register_value(reg, rb))
		return MCP2221_ERR_INVALID;

	mcp2221_i2c_byte_order_t byte_order;
	mcp2221_error_code_t err = resolve_byte_order(slave, reg_byteorder, &byte_order);
	if (err != MCP2221_ERR_OK)
		return err;

	uint8_t tmp[MCP2221_I2C_SLAVE_MAX_REGISTER_BYTES + MCP2221_I2C_SLAVE_MAX_TRANSFER_BYTES];
	encode_register(reg, rb, byte_order, tmp);
	if (length > 0)
		memcpy(tmp + rb, data, length);

	// normal write
	return mcp2221_i2c_write_ex(slave->mcp, slave->addr, tmp, rb + length, MCP2221_I2C_KIND_NORMAL, 50);
}

mcp2221_error_code_t mcp2221_i2c_slave_write(mcp2221_i2c_slave_t *slave, const uint8_t *data, size_t length) {
	if (!is_valid_slave(slave) || !data || !is_valid_transfer_length(length))
		return MCP2221_ERR_INVALID;

	return mcp2221_i2c_write_ex(slave->mcp, slave->addr, data, length, MCP2221_I2C_KIND_NORMAL, 50);
}
