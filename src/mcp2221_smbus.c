#include "smbus.h"

#include <string.h>

#include "mcp2221.h"

mcp2221_error_code_t mcp2221_smbus_init(mcp2221_smbus_t *bus, mcp2221_t *existing_mcp, int device_index, uint16_t vid, uint16_t pid, const char *usbserial,
			   uint32_t i2c_speed_hz) {
	if (!bus)
		return MCP2221_ERR_INVALID;

	bus->mcp = NULL;
	bus->owns_mcp = 0;

	if (existing_mcp != NULL) {
		bus->mcp = existing_mcp;
		return MCP2221_ERR_OK;
	}

	uint32_t target_i2c_speed_hz = i2c_speed_hz > 0 ? i2c_speed_hz : 100000;

	/* mcp2221_open_simple() follows EasyMCP2221's sequence: initialize the
	 * device at 100 kHz first, then apply target_i2c_speed_hz if needed.
	 */
	bus->mcp = mcp2221_open_simple(vid, pid, device_index, usbserial, target_i2c_speed_hz);
	if (!bus->mcp)
		return MCP2221_ERR_USB;
	bus->owns_mcp = 1;

	return MCP2221_ERR_OK;
}

void mcp2221_smbus_close(mcp2221_smbus_t *bus) {
	if (!bus)
		return;

	if (bus->owns_mcp && bus->mcp)
		mcp2221_close(bus->mcp);

	bus->mcp = NULL;
	bus->owns_mcp = 0;
}

// Internal helpers: register read/write
static mcp2221_error_code_t read_register(mcp2221_smbus_t *bus, uint8_t addr, uint32_t reg, int reg_bytes, uint8_t *buffer, size_t len) {
	uint8_t regbuf[4];

	for (int i = reg_bytes - 1; i >= 0; i--) {
		regbuf[i] = reg & 0xFF;
		reg >>= 8;
	}

	mcp2221_error_code_t err = mcp2221_i2c_write_simple(bus->mcp, addr, regbuf, reg_bytes, MCP2221_I2C_KIND_NO_STOP);
	if (err != MCP2221_ERR_OK)
		return err;

	return mcp2221_i2c_read_simple(bus->mcp, addr, buffer, len, MCP2221_I2C_KIND_REPEATED_START);
}

static mcp2221_error_code_t write_register(mcp2221_smbus_t *bus, uint8_t addr, uint32_t reg, int reg_bytes, const uint8_t *data, size_t len) {
	uint8_t temp[4 + MCP2221_I2C_SMBUS_BLOCK_MAX];
	for (int i = reg_bytes - 1; i >= 0; i--) {
		temp[i] = reg & 0xFF;
		reg >>= 8;
	}
	memcpy(&temp[reg_bytes], data, len);

	return mcp2221_i2c_write_simple(bus->mcp, addr, temp, reg_bytes + len, MCP2221_I2C_KIND_NORMAL);
}

// Basic smbus
mcp2221_error_code_t mcp2221_smbus_read_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t *value) {
	return mcp2221_i2c_read_simple(bus->mcp, addr, value, 1, MCP2221_I2C_KIND_NORMAL);
}

mcp2221_error_code_t mcp2221_smbus_write_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t value) {
	return mcp2221_i2c_write_simple(bus->mcp, addr, &value, 1, MCP2221_I2C_KIND_NORMAL);
}

mcp2221_error_code_t mcp2221_smbus_read_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *value) {
	return read_register(bus, addr, reg, 1, value, 1);
}

mcp2221_error_code_t mcp2221_smbus_write_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t value) {
	return write_register(bus, addr, reg, 1, &value, 1);
}

mcp2221_error_code_t mcp2221_smbus_read_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t *value) {
	uint8_t buf[2];
	mcp2221_error_code_t err = read_register(bus, addr, reg, 1, buf, 2);
	if (err != MCP2221_ERR_OK)
		return err;

	// Match EasyMCP2221.smbus.py which uses struct.unpack("h", ...) (native little-endian on x86)
	*value = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_smbus_write_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value) {
	uint8_t buf[2];
	buf[0] = value & 0xFF;
	buf[1] = (value >> 8) & 0xFF;

	return write_register(bus, addr, reg, 1, buf, 2);
}

mcp2221_error_code_t mcp2221_smbus_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response) {
	uint8_t buf[2];
	buf[0] = value & 0xFF;
	buf[1] = (value >> 8) & 0xFF;

	mcp2221_error_code_t err = mcp2221_i2c_write_simple(bus->mcp, addr, (uint8_t[]){reg, buf[0], buf[1]}, 3, MCP2221_I2C_KIND_NO_STOP);
	if (err != MCP2221_ERR_OK)
		return err;

	uint8_t resp[2];
	err = mcp2221_i2c_read_simple(bus->mcp, addr, resp, 2, MCP2221_I2C_KIND_REPEATED_START);
	if (err != MCP2221_ERR_OK)
		return err;

	*response = (int16_t)(resp[0] | ((uint16_t)resp[1] << 8));
	return MCP2221_ERR_OK;
}

// Block operations

mcp2221_error_code_t mcp2221_smbus_read_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length) {
	uint8_t temp[MCP2221_I2C_SMBUS_BLOCK_MAX + 1];
	mcp2221_error_code_t err = read_register(bus, addr, reg, 1, temp, MCP2221_I2C_SMBUS_BLOCK_MAX + 1);
	if (err != MCP2221_ERR_OK)
		return err;

	size_t len = temp[0];
	if (len > MCP2221_I2C_SMBUS_BLOCK_MAX)
		return MCP2221_ERR_INVALID;

	memcpy(buffer, &temp[1], len);
	*length = len;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_smbus_write_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length) {
	if (length > MCP2221_I2C_SMBUS_BLOCK_MAX)
		return MCP2221_ERR_INVALID;

	uint8_t temp[1 + MCP2221_I2C_SMBUS_BLOCK_MAX];
	temp[0] = (uint8_t)length;
	memcpy(&temp[1], data, length);

	return write_register(bus, addr, reg, 1, temp, 1 + length);
}

mcp2221_error_code_t mcp2221_smbus_block_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length,
							 uint8_t *response, size_t *resp_len) {
	if (length > MCP2221_I2C_SMBUS_BLOCK_MAX)
		return MCP2221_ERR_INVALID;

	// Python: I2C_write(addr, register + bytes([len]) + data, kind='nonstop')
	uint8_t txbuf[2 + MCP2221_I2C_SMBUS_BLOCK_MAX];
	txbuf[0] = reg;
	txbuf[1] = (uint8_t)length;
	memcpy(&txbuf[2], data, length);

	mcp2221_error_code_t err = mcp2221_i2c_write_simple(bus->mcp, addr, txbuf, 2 + length, MCP2221_I2C_KIND_NO_STOP);
	if (err != MCP2221_ERR_OK)
		return err;

	// Read response
	uint8_t rxbuf[MCP2221_I2C_SMBUS_BLOCK_MAX + 1];
	err = mcp2221_i2c_read_simple(bus->mcp, addr, rxbuf, MCP2221_I2C_SMBUS_BLOCK_MAX + 1, MCP2221_I2C_KIND_REPEATED_START);
	if (err != MCP2221_ERR_OK)
		return err;

	size_t len = rxbuf[0];
	if (len > MCP2221_I2C_SMBUS_BLOCK_MAX)
		return MCP2221_ERR_INVALID;
	memcpy(response, &rxbuf[1], len);
	*resp_len = len;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_smbus_read_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length) {
	if (length > MCP2221_I2C_SMBUS_BLOCK_MAX)
		return MCP2221_ERR_INVALID;
	return read_register(bus, addr, reg, 1, buffer, length);
}

mcp2221_error_code_t mcp2221_smbus_write_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length) {
	if (length > MCP2221_I2C_SMBUS_BLOCK_MAX)
		return MCP2221_ERR_INVALID;
	return write_register(bus, addr, reg, 1, data, length);
}


