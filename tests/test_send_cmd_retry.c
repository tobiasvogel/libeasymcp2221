#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <libusb.h>

#include "mcp2221_constants.h"
#include "mcp2221_internal_constants.h"
#include "mcp2221_flash.h"
#include "mcp2221_flash_settings.h"
#include "mcp2221_gpio_poll.h"
#include "mcp2221_smbus.h"

static int mock_write_count;
static int mock_read_count;
static int mock_mode;
static uint8_t mock_last_cmd;

enum {
	MOCK_READ_TIMEOUT = 1,
	MOCK_TIMEOUT_THEN_OK,
	MOCK_PROTOCOL_ERROR,
	MOCK_I2C_GET_DATA_NACK,
	MOCK_I2C_GET_DATA_UNKNOWN_ERROR,
	MOCK_I2C_GET_DATA_TIMEOUT,
	MOCK_FLASH_COMMAND_FAILURE
};

int libusb_interrupt_transfer(libusb_device_handle *dev_handle, unsigned char endpoint,
							  unsigned char *data, int length, int *transferred,
							  unsigned int timeout) {
	(void)dev_handle;
	(void)timeout;

	assert(length == MCP2221_PACKET_SIZE);

	if ((endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
		mock_write_count++;
		mock_last_cmd = data[0];
		*transferred = length;
		return 0;
	}

	mock_read_count++;

	if (mock_mode == MOCK_READ_TIMEOUT) {
		*transferred = 0;
		return LIBUSB_ERROR_TIMEOUT;
	}

	if (mock_mode == MOCK_TIMEOUT_THEN_OK && mock_read_count == 1) {
		*transferred = 0;
		return LIBUSB_ERROR_TIMEOUT;
	}

	memset(data, 0, (size_t)length);

	if (mock_mode == MOCK_FLASH_COMMAND_FAILURE) {
		data[MCP2221_RESPONSE_ECHO_BYTE] = mock_last_cmd;
		data[MCP2221_RESPONSE_STATUS_BYTE] = 0x01;
		*transferred = length;
		return 0;
	}

	if (mock_mode == MOCK_I2C_GET_DATA_NACK ||
	    mock_mode == MOCK_I2C_GET_DATA_UNKNOWN_ERROR ||
	    mock_mode == MOCK_I2C_GET_DATA_TIMEOUT) {
		data[MCP2221_RESPONSE_ECHO_BYTE] = mock_last_cmd;
		data[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;

		if (mock_last_cmd == MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA) {
			data[MCP2221_RESPONSE_STATUS_BYTE] = 0x41;
			if (mock_mode == MOCK_I2C_GET_DATA_NACK)
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] =
					MCP2221_I2C_ST_WRADDRL_NACK_STOP;
			else if (mock_mode == MOCK_I2C_GET_DATA_TIMEOUT)
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] =
					MCP2221_I2C_ST_WRADDRL_TOUT;
			else
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] = 0xFF;
		}

		*transferred = length;
		return 0;
	}

	data[MCP2221_RESPONSE_ECHO_BYTE] =
		(mock_mode == MOCK_PROTOCOL_ERROR)
			? MCP2221_CMD_GET_GPIO_VALUES
			: MCP2221_CMD_GET_SRAM_SETTINGS;
	data[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	*transferred = length;
	return 0;
}

/*
 * Include the implementation directly so this unit test can construct the
 * otherwise opaque device object. libusb_interrupt_transfer() above is the
 * only transport primitive exercised by mcp2221_send_cmd().
 */
#include "../src/mcp2221.c"

static void reset_mock(int mode) {
	mock_write_count = 0;
	mock_read_count = 0;
	mock_mode = mode;
	mock_last_cmd = 0;
}

static mcp2221_t make_test_device(void) {
	mcp2221_t dev;
	memset(&dev, 0, sizeof(dev));
	dev.handle = (libusb_device_handle *)(uintptr_t)1;
	dev.ep_out = MCP2221_DEFAULT_EP_OUT;
	dev.ep_in = MCP2221_DEFAULT_EP_IN;
	dev.usb_read_timeout_ms = 10;
	dev.cmd_retries = 3;
	return dev;
}

static void test_public_send_is_single_shot(void) {
	mcp2221_t dev = make_test_device();
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t response[MCP2221_PACKET_SIZE];

	reset_mock(MOCK_READ_TIMEOUT);

	assert(mcp2221_send_cmd(&dev, &cmd, 1, response) == MCP2221_ERR_TIMEOUT);
	assert(mock_write_count == 1);
	assert(mock_read_count == 1);
}

static void test_retry_safe_retries_timeout(void) {
	mcp2221_t dev = make_test_device();
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t response[MCP2221_PACKET_SIZE];

	reset_mock(MOCK_TIMEOUT_THEN_OK);

	assert(mcp2221_internal_send_cmd_retry_safe(&dev, &cmd, 1, response) == MCP2221_ERR_OK);
	assert(mock_write_count == 2);
	assert(mock_read_count == 2);
	assert(response[MCP2221_RESPONSE_ECHO_BYTE] == cmd);
}

static void test_retry_safe_does_not_retry_protocol_error(void) {
	mcp2221_t dev = make_test_device();
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t response[MCP2221_PACKET_SIZE];

	reset_mock(MOCK_PROTOCOL_ERROR);

	assert(mcp2221_internal_send_cmd_retry_safe(&dev, &cmd, 1, response) == MCP2221_ERR_PROTOCOL);
	assert(mock_write_count == 1);
	assert(mock_read_count == 1);
}


static void test_i2c_command_failure_maps_nack(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data;

	reset_mock(MOCK_I2C_GET_DATA_NACK);

	assert(mcp2221_i2c_read_ex(
		&dev,
		0x50,
		&data,
		1,
		MCP2221_I2C_KIND_NORMAL,
		100) == MCP2221_ERR_NOT_ACK);
}


static void test_i2c_address_timeout_maps_nack(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data;

	reset_mock(MOCK_I2C_GET_DATA_TIMEOUT);

	assert(mcp2221_i2c_read_ex(
		&dev,
		0x50,
		&data,
		1,
		MCP2221_I2C_KIND_NORMAL,
		100) == MCP2221_ERR_NOT_ACK);
}

static void test_i2c_command_failure_maps_unknown_state(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data;

	reset_mock(MOCK_I2C_GET_DATA_UNKNOWN_ERROR);

	assert(mcp2221_i2c_read_ex(
		&dev,
		0x50,
		&data,
		1,
		MCP2221_I2C_KIND_NORMAL,
		100) == MCP2221_ERR_I2C);
}


static void test_flash_read_command_failure_maps_flash_read(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data[60];

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_flash_read(
		&dev,
		MCP2221_FLASH_DATA_CHIP_SETTINGS,
		data) == MCP2221_ERR_FLASH_READ);
}

static void test_flash_write_command_failure_maps_flash_write(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data[60] = {0};

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_flash_write(
		&dev,
		MCP2221_FLASH_DATA_CHIP_SETTINGS,
		data) == MCP2221_ERR_FLASH_WRITE);
}

static void test_flash_password_command_failure_maps_flash_password(void) {
	mcp2221_t dev = make_test_device();
	uint8_t password[8] = {0};

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_flash_send_password(
		&dev,
		password) == MCP2221_ERR_FLASH_PASSWD);
}


static void test_flash_rejects_null_arguments(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data[60] = {0};
	uint8_t password[8] = {0};

	reset_mock(0);

	assert(mcp2221_flash_read(NULL, MCP2221_FLASH_DATA_CHIP_SETTINGS, data) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_flash_read(&dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, NULL) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_flash_write(NULL, MCP2221_FLASH_DATA_CHIP_SETTINGS, data) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_flash_write(&dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, NULL) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_flash_send_password(NULL, password) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_flash_send_password(&dev, NULL) ==
	       MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

static void test_flash_settings_rejects_null_arguments(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_flash_settings_t settings;

	reset_mock(0);

	assert(mcp2221_flash_get_settings(NULL, &settings) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_flash_get_settings(&dev, NULL) ==
	       MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

static void test_gpio_poll_rejects_null_arguments(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_gpio_poll_state_t state;
	mcp2221_gpio_change_t changes[4];

	reset_mock(0);

	mcp2221_gpio_poll_init(NULL);

	assert(mcp2221_gpio_poll(NULL, &state, changes) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_gpio_poll(&dev, NULL, changes) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_gpio_poll(&dev, &state, NULL) ==
	       MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

static void test_smbus_rejects_invalid_context_and_pointers(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_smbus_t invalid_bus = {0};
	mcp2221_smbus_t bus = {
		.mcp = &dev,
		.owns_mcp = 0
	};
	uint8_t byte_value;
	size_t length;
	uint8_t buffer[MCP2221_I2C_SMBUS_BLOCK_MAX];

	reset_mock(0);

	assert(mcp2221_smbus_read_byte(NULL, 0x50, &byte_value) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_read_byte(&invalid_bus, 0x50, &byte_value) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_write_byte(&invalid_bus, 0x50, 0) ==
	       MCP2221_ERR_INVALID);

	assert(mcp2221_smbus_read_byte(&bus, 0x50, NULL) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_read_byte_data(&bus, 0x50, 0, NULL) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_read_word_data(&bus, 0x50, 0, NULL) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_process_call(&bus, 0x50, 0, 0, NULL) ==
	       MCP2221_ERR_INVALID);

	assert(mcp2221_smbus_read_block_data(&bus, 0x50, 0, NULL, &length) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_read_block_data(&bus, 0x50, 0, buffer, NULL) ==
	       MCP2221_ERR_INVALID);

	assert(mcp2221_smbus_write_block_data(&bus, 0x50, 0, NULL, 1) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_block_process_call(
		       &bus, 0x50, 0, NULL, 1, buffer, &length) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_block_process_call(
		       &bus, 0x50, 0, buffer, 1, NULL, &length) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_block_process_call(
		       &bus, 0x50, 0, buffer, 1, buffer, NULL) ==
	       MCP2221_ERR_INVALID);

	assert(mcp2221_smbus_read_i2c_block_data(&bus, 0x50, 0, NULL, 1) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_smbus_write_i2c_block_data(&bus, 0x50, 0, NULL, 1) ==
	       MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

int main(void) {
	test_public_send_is_single_shot();
	test_retry_safe_retries_timeout();
	test_retry_safe_does_not_retry_protocol_error();
	test_i2c_command_failure_maps_nack();
	test_i2c_address_timeout_maps_nack();
	test_i2c_command_failure_maps_unknown_state();
	test_flash_read_command_failure_maps_flash_read();
	test_flash_write_command_failure_maps_flash_write();
	test_flash_password_command_failure_maps_flash_password();
	test_flash_rejects_null_arguments();
	test_flash_settings_rejects_null_arguments();
	test_gpio_poll_rejects_null_arguments();
	test_smbus_rejects_invalid_context_and_pointers();
	return 0;
}
