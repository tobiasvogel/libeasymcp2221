#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mcp2221.h"
#include "mcp2221_smbus.h"

struct mcp2221_device {
	int unused;
};

static uint8_t captured_write[8];
static size_t captured_write_len;
static mcp2221_i2c_kind_t captured_write_kind;
static uint8_t read_response[2];

mcp2221_error_code_t mcp2221_open_simple(
	uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
	int i2c_speed_hz, mcp2221_t **out_dev) {
	(void)vid;
	(void)pid;
	(void)devnum;
	(void)usbserial;
	(void)i2c_speed_hz;
	(void)out_dev;
	return MCP2221_ERR_INVALID;
}

void mcp2221_close(mcp2221_t *dev) {
	(void)dev;
}

mcp2221_error_code_t mcp2221_i2c_write_simple(
	mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len,
	mcp2221_i2c_kind_t kind) {
	(void)dev;
	(void)addr;
	assert(len <= sizeof(captured_write));
	memcpy(captured_write, data, len);
	captured_write_len = len;
	captured_write_kind = kind;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_i2c_read_simple(
	mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len,
	mcp2221_i2c_kind_t kind) {
	(void)dev;
	(void)addr;
	assert(kind == MCP2221_I2C_KIND_REPEATED_START);
	assert(len == sizeof(read_response));
	memcpy(data, read_response, len);
	return MCP2221_ERR_OK;
}

static void assert_word_encoding(int16_t value, uint8_t low, uint8_t high) {
	struct mcp2221_device dev = {0};
	mcp2221_smbus_t bus = {
		.mcp = &dev,
		.owns_mcp = 0
	};

	memset(captured_write, 0, sizeof(captured_write));
	captured_write_len = 0;

	assert(mcp2221_smbus_write_word_data(
		&bus, 0x50, 0x2a, value) == MCP2221_ERR_OK);
	assert(captured_write_len == 3);
	assert(captured_write_kind == MCP2221_I2C_KIND_NORMAL);
	assert(captured_write[0] == 0x2a);
	assert(captured_write[1] == low);
	assert(captured_write[2] == high);
}

static void test_write_word_encoding(void) {
	assert_word_encoding((int16_t)0x1234, 0x34, 0x12);
	assert_word_encoding((int16_t)-1, 0xff, 0xff);
	assert_word_encoding((int16_t)INT16_MIN, 0x00, 0x80);
}

static void test_process_call_word_encoding_and_decode(void) {
	struct mcp2221_device dev = {0};
	mcp2221_smbus_t bus = {
		.mcp = &dev,
		.owns_mcp = 0
	};
	int16_t response = 0;

	read_response[0] = 0x34;
	read_response[1] = 0x92;

	assert(mcp2221_smbus_process_call(
		&bus, 0x50, 0x17, INT16_MIN, &response) == MCP2221_ERR_OK);
	assert(captured_write_len == 3);
	assert(captured_write_kind == MCP2221_I2C_KIND_NO_STOP);
	assert(captured_write[0] == 0x17);
	assert(captured_write[1] == 0x00);
	assert(captured_write[2] == 0x80);
	assert((uint16_t)response == 0x9234u);
}

int main(void) {
	test_write_word_encoding();
	test_process_call_word_encoding_and_decode();
	return 0;
}
