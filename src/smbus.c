#include "smbus.h"

#include <string.h>

#include "mcp2221.h"

int smbus_init(SMBus *bus, MCP2221 *existing_mcp, int device_index, uint16_t vid, uint16_t pid, const char *usbserial,
			   uint32_t clock_hz) {
	if (existing_mcp != NULL) {
		bus->mcp = existing_mcp;
		return 0;
	}

	// Create + initialize new MCP2221 device
	bus->mcp = mcp2221_open(vid, pid, device_index, usbserial);
	if (!bus->mcp)
		return -1;

	if (mcp2221_i2c_set_speed(bus->mcp, clock_hz) != 0)
		return -2;

	return 0;
}

// Internal helpers: register read/write
static int read_register(SMBus *bus, uint8_t addr, uint32_t reg, int reg_bytes, uint8_t *buffer, size_t len) {
	uint8_t regbuf[4];

	for (int i = reg_bytes - 1; i >= 0; i--) {
		regbuf[i] = reg & 0xFF;
		reg >>= 8;
	}

	int r = mcp2221_i2c_write(bus->mcp, addr, regbuf, reg_bytes, 1);
	if (r != 0)
		return r;

	return mcp2221_i2c_read(bus->mcp, addr, buffer, len, 1);
}

static int write_register(SMBus *bus, uint8_t addr, uint32_t reg, int reg_bytes, const uint8_t *data, size_t len) {
	uint8_t temp[4 + I2C_SMBUS_BLOCK_MAX];
	for (int i = reg_bytes - 1; i >= 0; i--) {
		temp[i] = reg & 0xFF;
		reg >>= 8;
	}
	memcpy(&temp[reg_bytes], data, len);

	return mcp2221_i2c_write(bus->mcp, addr, temp, reg_bytes + len, 0);
}

// ---------------- BASIC SMBUS ----------------

int smbus_read_byte(SMBus *bus, uint8_t addr, uint8_t *value) {
	return mcp2221_i2c_read(bus->mcp, addr, value, 1, 0);
}

int smbus_write_byte(SMBus *bus, uint8_t addr, uint8_t value) {
	return mcp2221_i2c_write(bus->mcp, addr, &value, 1, 0);
}

int smbus_read_byte_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *value) {
	return read_register(bus, addr, reg, 1, value, 1);
}

int smbus_write_byte_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t value) {
	return write_register(bus, addr, reg, 1, &value, 1);
}

int smbus_read_word_data(SMBus *bus, uint8_t addr, uint8_t reg, int16_t *value) {
	uint8_t buf[2];
	int r = read_register(bus, addr, reg, 1, buf, 2);
	if (r != 0)
		return r;

	*value = (int16_t)((buf[0] << 8) | buf[1]);
	return 0;
}

int smbus_write_word_data(SMBus *bus, uint8_t addr, uint8_t reg, int16_t value) {
	uint8_t buf[2];
	buf[0] = (value >> 8) & 0xFF;
	buf[1] = value & 0xFF;

	return write_register(bus, addr, reg, 1, buf, 2);
}

int smbus_process_call(SMBus *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response) {
	uint8_t buf[2];
	buf[0] = (value >> 8) & 0xFF;
	buf[1] = value & 0xFF;

	int r = mcp2221_i2c_write(bus->mcp, addr, (uint8_t[]){reg, buf[0], buf[1]}, 3, 1);
	if (r != 0)
		return r;

	uint8_t resp[2];
	r = mcp2221_i2c_read(bus->mcp, addr, resp, 2, 1);
	if (r != 0)
		return r;

	*response = (resp[0] << 8) | resp[1];
	return 0;
}

// ---------------- BLOCK OPERATIONS ----------------

int smbus_read_block_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length) {
	uint8_t temp[I2C_SMBUS_BLOCK_MAX];
	int r = read_register(bus, addr, reg, 1, temp, I2C_SMBUS_BLOCK_MAX);
	if (r != 0)
		return r;

	size_t len = temp[0];
	if (len > I2C_SMBUS_BLOCK_MAX)
		return -3;

	memcpy(buffer, &temp[1], len);
	*length = len;

	return 0;
}

int smbus_write_block_data(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length) {
	if (length > I2C_SMBUS_BLOCK_MAX)
		return -1;

	uint8_t temp[1 + I2C_SMBUS_BLOCK_MAX];
	temp[0] = (uint8_t)length;
	memcpy(&temp[1], data, length);

	return write_register(bus, addr, reg, 1, temp, 1 + length);
}

int smbus_block_process_call(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length,
							 uint8_t *response, size_t *resp_len) {
	uint8_t header[2] = {reg, (uint8_t)length};

	// Send register + length + data (nonstop)
	int r = mcp2221_i2c_write(bus->mcp, addr, NULL, 0, 0);
	r = mcp2221_i2c_write(bus->mcp, addr, header, 2, 1);
	if (r != 0)
		return r;
	r = mcp2221_i2c_write(bus->mcp, addr, data, length, 1);
	if (r != 0)
		return r;

	// Read response
	uint8_t temp[I2C_SMBUS_BLOCK_MAX];
	r = mcp2221_i2c_read(bus->mcp, addr, temp, I2C_SMBUS_BLOCK_MAX, 1);
	if (r != 0)
		return r;

	size_t len = temp[0];
	memcpy(response, &temp[1], len);
	*resp_len = len;

	return 0;
}

int smbus_read_i2c_block_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length) {
	if (length > I2C_SMBUS_BLOCK_MAX)
		return -1;
	return read_register(bus, addr, reg, 1, buffer, length);
}

int smbus_write_i2c_block_data(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length) {
	if (length > I2C_SMBUS_BLOCK_MAX)
		return -1;
	return write_register(bus, addr, reg, 1, data, length);
}
