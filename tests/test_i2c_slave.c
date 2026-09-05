#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mcp2221.h"
#include "mcp2221_i2c_slave.h"

struct mcp2221_device {
	int unused;
};

enum {
	TEST_ADDR = 0x50,
	TEST_TIMEOUT_MS = 50,
	TEST_MAX_CAPTURE = 260
};

static int speed_calls;
static uint32_t captured_speed;
static mcp2221_error_code_t speed_result;

static int write_calls;
static uint8_t captured_write_addr;
static uint8_t captured_write[TEST_MAX_CAPTURE];
static size_t captured_write_len;
static mcp2221_i2c_kind_t captured_write_kind;
static int captured_write_timeout;
static mcp2221_error_code_t write_result;

static int read_calls;
static uint8_t captured_read_addr;
static size_t captured_read_len;
static mcp2221_i2c_kind_t captured_read_kind;
static int captured_read_timeout;
static uint8_t read_data[256];
static mcp2221_error_code_t read_result;

static void reset_capture(void) {
	speed_calls = 0;
	captured_speed = 0;
	speed_result = MCP2221_ERR_OK;

	write_calls = 0;
	captured_write_addr = 0;
	memset(captured_write, 0, sizeof(captured_write));
	captured_write_len = 0;
	captured_write_kind = MCP2221_I2C_KIND_NORMAL;
	captured_write_timeout = 0;
	write_result = MCP2221_ERR_OK;

	read_calls = 0;
	captured_read_addr = 0;
	captured_read_len = 0;
	captured_read_kind = MCP2221_I2C_KIND_NORMAL;
	captured_read_timeout = 0;
	memset(read_data, 0, sizeof(read_data));
	read_result = MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_i2c_set_speed(
	mcp2221_t *dev, uint32_t i2c_speed_hz) {
	assert(dev != NULL);
	speed_calls++;
	captured_speed = i2c_speed_hz;
	return speed_result;
}

mcp2221_error_code_t mcp2221_i2c_write_ex(
	mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len,
	mcp2221_i2c_kind_t kind, int i2c_timeout_ms) {
	assert(dev != NULL);
	assert(data != NULL);
	assert(len <= sizeof(captured_write));
	write_calls++;
	captured_write_addr = addr;
	memcpy(captured_write, data, len);
	captured_write_len = len;
	captured_write_kind = kind;
	captured_write_timeout = i2c_timeout_ms;
	return write_result;
}

mcp2221_error_code_t mcp2221_i2c_read_ex(
	mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len,
	mcp2221_i2c_kind_t kind, int i2c_timeout_ms) {
	assert(dev != NULL);
	assert(data != NULL);
	assert(len <= sizeof(read_data));
	read_calls++;
	captured_read_addr = addr;
	captured_read_len = len;
	captured_read_kind = kind;
	captured_read_timeout = i2c_timeout_ms;
	if (read_result == MCP2221_ERR_OK)
		memcpy(data, read_data, len);
	return read_result;
}

static mcp2221_i2c_slave_t make_slave(
	mcp2221_t *dev, int reg_bytes, mcp2221_i2c_byte_order_t order) {
	mcp2221_i2c_slave_t slave = {
		.mcp = dev,
		.addr = TEST_ADDR,
		.reg_bytes = reg_bytes,
		.reg_byteorder = order
	};
	return slave;
}

static void test_init_defaults_and_force_skips_presence_probe(void) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave;

	reset_capture();
	memset(&slave, 0xA5, sizeof(slave));

	assert(mcp2221_i2c_slave_init(
		&slave, &dev, TEST_ADDR, 1, 100000u,
		0, MCP2221_I2C_BYTE_ORDER_DEFAULT) == MCP2221_ERR_OK);

	assert(speed_calls == 1);
	assert(captured_speed == 100000u);
	assert(read_calls == 0);
	assert(write_calls == 0);
	assert(slave.mcp == &dev);
	assert(slave.addr == TEST_ADDR);
	assert(slave.reg_bytes == 1);
	assert(slave.reg_byteorder == MCP2221_I2C_BYTE_ORDER_BIG);
}

static void test_init_presence_probe_and_failure_leave_invalid_context(void) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave;

	reset_capture();
	assert(mcp2221_i2c_slave_init(
		&slave, &dev, TEST_ADDR, 0, 400000u,
		2, MCP2221_I2C_BYTE_ORDER_LITTLE) == MCP2221_ERR_OK);
	assert(speed_calls == 1);
	assert(read_calls == 1);
	assert(captured_read_addr == TEST_ADDR);
	assert(captured_read_len == 1);
	assert(captured_read_kind == MCP2221_I2C_KIND_NORMAL);
	assert(captured_read_timeout == TEST_TIMEOUT_MS);
	assert(slave.mcp == &dev);
	assert(slave.reg_bytes == 2);
	assert(slave.reg_byteorder == MCP2221_I2C_BYTE_ORDER_LITTLE);

	reset_capture();
	read_result = MCP2221_ERR_NOT_ACK;
	memset(&slave, 0xA5, sizeof(slave));
	assert(mcp2221_i2c_slave_init(
		&slave, &dev, TEST_ADDR, 0, 100000u,
		1, MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_NOT_ACK);
	assert(speed_calls == 1);
	assert(read_calls == 1);
	assert(slave.mcp == NULL);

	reset_capture();
	speed_result = MCP2221_ERR_TIMEOUT;
	memset(&slave, 0xA5, sizeof(slave));
	assert(mcp2221_i2c_slave_init(
		&slave, &dev, TEST_ADDR, 1, 100000u,
		1, MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_TIMEOUT);
	assert(speed_calls == 1);
	assert(read_calls == 0);
	assert(slave.mcp == NULL);
}

static void assert_read_register_encoding(
	int bytes, mcp2221_i2c_byte_order_t order,
	uint32_t reg, const uint8_t *expected) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave = make_slave(&dev, 1, MCP2221_I2C_BYTE_ORDER_BIG);
	uint8_t out[3] = {0};

	reset_capture();
	read_data[0] = 0x11;
	read_data[1] = 0x22;
	read_data[2] = 0x33;

	assert(mcp2221_i2c_slave_read_register(
		&slave, reg, out, sizeof(out), bytes, order) == MCP2221_ERR_OK);

	assert(write_calls == 1);
	assert(captured_write_addr == TEST_ADDR);
	assert(captured_write_len == (size_t)bytes);
	assert(memcmp(captured_write, expected, (size_t)bytes) == 0);
	assert(captured_write_kind == MCP2221_I2C_KIND_NO_STOP);
	assert(captured_write_timeout == TEST_TIMEOUT_MS);

	assert(read_calls == 1);
	assert(captured_read_addr == TEST_ADDR);
	assert(captured_read_len == sizeof(out));
	assert(captured_read_kind == MCP2221_I2C_KIND_REPEATED_START);
	assert(captured_read_timeout == TEST_TIMEOUT_MS);
	assert(out[0] == 0x11 && out[1] == 0x22 && out[2] == 0x33);
}

static void test_read_register_encodes_all_widths_and_orders(void) {
	const uint8_t be1[] = {0x7A};
	const uint8_t le1[] = {0x7A};
	const uint8_t be2[] = {0x12, 0x34};
	const uint8_t le2[] = {0x34, 0x12};
	const uint8_t be3[] = {0x12, 0x34, 0x56};
	const uint8_t le3[] = {0x56, 0x34, 0x12};
	const uint8_t be4[] = {0x12, 0x34, 0x56, 0x78};
	const uint8_t le4[] = {0x78, 0x56, 0x34, 0x12};

	assert_read_register_encoding(1, MCP2221_I2C_BYTE_ORDER_BIG, 0x7Au, be1);
	assert_read_register_encoding(1, MCP2221_I2C_BYTE_ORDER_LITTLE, 0x7Au, le1);
	assert_read_register_encoding(2, MCP2221_I2C_BYTE_ORDER_BIG, 0x1234u, be2);
	assert_read_register_encoding(2, MCP2221_I2C_BYTE_ORDER_LITTLE, 0x1234u, le2);
	assert_read_register_encoding(3, MCP2221_I2C_BYTE_ORDER_BIG, 0x123456u, be3);
	assert_read_register_encoding(3, MCP2221_I2C_BYTE_ORDER_LITTLE, 0x123456u, le3);
	assert_read_register_encoding(4, MCP2221_I2C_BYTE_ORDER_BIG, 0x12345678u, be4);
	assert_read_register_encoding(4, MCP2221_I2C_BYTE_ORDER_LITTLE, 0x12345678u, le4);
}

static void test_default_register_settings_are_used(void) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave =
		make_slave(&dev, 3, MCP2221_I2C_BYTE_ORDER_LITTLE);
	uint8_t out = 0;
	const uint8_t expected[] = {0x56, 0x34, 0x12};

	reset_capture();
	read_data[0] = 0xA5;

	assert(mcp2221_i2c_slave_read_register(
		&slave, 0x123456u, &out, 1,
		0, MCP2221_I2C_BYTE_ORDER_DEFAULT) == MCP2221_ERR_OK);
	assert(captured_write_len == sizeof(expected));
	assert(memcmp(captured_write, expected, sizeof(expected)) == 0);
	assert(out == 0xA5);
}

static void test_write_register_encodes_address_and_payload(void) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave =
		make_slave(&dev, 2, MCP2221_I2C_BYTE_ORDER_BIG);
	const uint8_t payload[] = {0xAA, 0x55, 0x00};
	const uint8_t expected[] = {0x12, 0x34, 0xAA, 0x55, 0x00};

	reset_capture();

	assert(mcp2221_i2c_slave_write_register(
		&slave, 0x1234u, payload, sizeof(payload),
		0, MCP2221_I2C_BYTE_ORDER_DEFAULT) == MCP2221_ERR_OK);

	assert(write_calls == 1);
	assert(read_calls == 0);
	assert(captured_write_addr == TEST_ADDR);
	assert(captured_write_len == sizeof(expected));
	assert(memcmp(captured_write, expected, sizeof(expected)) == 0);
	assert(captured_write_kind == MCP2221_I2C_KIND_NORMAL);
	assert(captured_write_timeout == TEST_TIMEOUT_MS);

	reset_capture();
	assert(mcp2221_i2c_slave_write_register(
		&slave, 0x4321u, NULL, 0,
		2, MCP2221_I2C_BYTE_ORDER_LITTLE) == MCP2221_ERR_OK);
	assert(captured_write_len == 2);
	assert(captured_write[0] == 0x21);
	assert(captured_write[1] == 0x43);
}

static void test_direct_read_write_and_presence_helpers(void) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave =
		make_slave(&dev, 1, MCP2221_I2C_BYTE_ORDER_BIG);
	uint8_t out[2] = {0};
	const uint8_t payload[] = {0xDE, 0xAD};
	int present = -1;

	reset_capture();
	read_data[0] = 0xCA;
	read_data[1] = 0xFE;
	assert(mcp2221_i2c_slave_read(&slave, out, sizeof(out)) == MCP2221_ERR_OK);
	assert(read_calls == 1);
	assert(captured_read_kind == MCP2221_I2C_KIND_NORMAL);
	assert(captured_read_timeout == TEST_TIMEOUT_MS);
	assert(out[0] == 0xCA && out[1] == 0xFE);

	reset_capture();
	assert(mcp2221_i2c_slave_write(
		&slave, payload, sizeof(payload)) == MCP2221_ERR_OK);
	assert(write_calls == 1);
	assert(captured_write_kind == MCP2221_I2C_KIND_NORMAL);
	assert(captured_write_timeout == TEST_TIMEOUT_MS);
	assert(captured_write_len == sizeof(payload));
	assert(memcmp(captured_write, payload, sizeof(payload)) == 0);

	reset_capture();
	read_result = MCP2221_ERR_NOT_ACK;
	assert(mcp2221_i2c_slave_check_present(&slave, &present) == MCP2221_ERR_OK);
	assert(present == 0);

	reset_capture();
	assert(mcp2221_i2c_slave_is_present(&slave) == 1);

	reset_capture();
	read_result = MCP2221_ERR_TIMEOUT;
	assert(mcp2221_i2c_slave_is_present(&slave) == 0);
}

static void test_invalid_arguments_do_not_start_io(void) {
	struct mcp2221_device dev = {0};
	mcp2221_i2c_slave_t slave =
		make_slave(&dev, 1, MCP2221_I2C_BYTE_ORDER_BIG);
	uint8_t byte = 0xA5;

	reset_capture();
	assert(mcp2221_i2c_slave_read_register(
		&slave, 0x100u, &byte, 1,
		1, MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_INVALID);
	assert(write_calls == 0 && read_calls == 0);

	assert(mcp2221_i2c_slave_read_register(
		&slave, 0, &byte, 0,
		1, MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_INVALID);
	assert(write_calls == 0 && read_calls == 0);

	assert(mcp2221_i2c_slave_write_register(
		&slave, 0, NULL, 1,
		1, MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_INVALID);
	assert(write_calls == 0 && read_calls == 0);

	assert(mcp2221_i2c_slave_read(
		&slave, &byte, 257u) == MCP2221_ERR_INVALID);
	assert(write_calls == 0 && read_calls == 0);

	assert(mcp2221_i2c_slave_write(
		&slave, &byte, 0) == MCP2221_ERR_INVALID);
	assert(write_calls == 0 && read_calls == 0);

	slave.reg_byteorder = (mcp2221_i2c_byte_order_t)99;
	assert(mcp2221_i2c_slave_write(
		&slave, &byte, 1) == MCP2221_ERR_INVALID);
	assert(write_calls == 0 && read_calls == 0);
}

int main(void) {
	test_init_defaults_and_force_skips_presence_probe();
	test_init_presence_probe_and_failure_leave_invalid_context();
	test_read_register_encodes_all_widths_and_orders();
	test_default_register_settings_are_used();
	test_write_register_encodes_address_and_payload();
	test_direct_read_write_and_presence_helpers();
	test_invalid_arguments_do_not_start_io();
	return 0;
}
