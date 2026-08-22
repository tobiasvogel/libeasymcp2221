#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <libusb.h>

#include "mcp2221_constants.h"
#include "mcp2221_internal_constants.h"
#include "mcp2221_flash.h"
#include "mcp2221_flash_info.h"
#include "mcp2221_flash_settings.h"
#include "mcp2221_gpio_poll.h"
#include "mcp2221_smbus.h"
#include "mcp2221_gpio.h"
#include "mcp2221_pin.h"
#include "mcp2221_sram.h"
#include "mcp2221_usb.h"

static int mock_write_count;
static int mock_read_count;
static int mock_sram_read_count;
static int mock_mode;
static uint8_t mock_last_cmd;

enum {
	MOCK_READ_TIMEOUT = 1,
	MOCK_TIMEOUT_THEN_OK,
	MOCK_PROTOCOL_ERROR,
	MOCK_I2C_GET_DATA_NACK,
	MOCK_I2C_GET_DATA_UNKNOWN_ERROR,
	MOCK_I2C_GET_DATA_TIMEOUT,
	MOCK_I2C_GET_DATA_OVERSIZED_CHUNK,
	MOCK_FLASH_COMMAND_FAILURE,
	MOCK_OPEN_INIT_NO_MEMORY,
	MOCK_OPEN_INIT_FAILURE,
	MOCK_OPEN_NOT_FOUND,
	MOCK_SRAM_TIMEOUT_THEN_OK,
	MOCK_I2C_SPEED_OK
};

int libusb_init(libusb_context **ctx) {
	if (mock_mode == MOCK_OPEN_INIT_NO_MEMORY) {
		if (ctx)
			*ctx = NULL;
		return LIBUSB_ERROR_NO_MEM;
	}
	if (mock_mode == MOCK_OPEN_INIT_FAILURE) {
		if (ctx)
			*ctx = NULL;
		return LIBUSB_ERROR_OTHER;
	}

	if (ctx)
		*ctx = (libusb_context *)(uintptr_t)1;
	return 0;
}

void libusb_exit(libusb_context *ctx) {
	(void)ctx;
}

ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list) {
	(void)ctx;

	if (list)
		*list = NULL;
	return 0;
}

void libusb_free_device_list(libusb_device **list, int unref_devices) {
	(void)list;
	(void)unref_devices;
}

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

	if (mock_last_cmd == MCP2221_CMD_GET_SRAM_SETTINGS)
		mock_sram_read_count++;

	if (mock_mode == MOCK_READ_TIMEOUT) {
		*transferred = 0;
		return LIBUSB_ERROR_TIMEOUT;
	}

	if (mock_mode == MOCK_TIMEOUT_THEN_OK && mock_read_count == 1) {
		*transferred = 0;
		return LIBUSB_ERROR_TIMEOUT;
	}

	if (mock_mode == MOCK_SRAM_TIMEOUT_THEN_OK &&
	    mock_last_cmd == MCP2221_CMD_GET_SRAM_SETTINGS &&
	    mock_sram_read_count == 1) {
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
	    mock_mode == MOCK_I2C_GET_DATA_TIMEOUT ||
	    mock_mode == MOCK_I2C_GET_DATA_OVERSIZED_CHUNK) {
		data[MCP2221_RESPONSE_ECHO_BYTE] = mock_last_cmd;
		data[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;

		if (mock_last_cmd == MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA) {
			if (mock_mode != MOCK_I2C_GET_DATA_OVERSIZED_CHUNK)
				data[MCP2221_RESPONSE_STATUS_BYTE] = 0x41;

			if (mock_mode == MOCK_I2C_GET_DATA_NACK)
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] =
					MCP2221_I2C_ST_WRADDRL_NACK_STOP;
			else if (mock_mode == MOCK_I2C_GET_DATA_TIMEOUT)
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] =
					MCP2221_I2C_ST_WRADDRL_TOUT;
			else if (mock_mode == MOCK_I2C_GET_DATA_OVERSIZED_CHUNK) {
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] = MCP2221_I2C_ST_READDATA_WAITGET;
				data[3] = MCP2221_I2C_CHUNK_SIZE + 1;
			}
			else
				data[MCP2221_I2C_INTERNAL_STATUS_BYTE] = 0xFF;
		}

		*transferred = length;
		return 0;
	}

	if (mock_mode == MOCK_I2C_SPEED_OK) {
		data[MCP2221_RESPONSE_ECHO_BYTE] = mock_last_cmd;
		data[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
		data[MCP2221_I2C_POLL_RESP_NEWSPEED_STATUS] = MCP2221_I2C_NEWSPEED_ACCEPTED;
		*transferred = length;
		return 0;
	}

	data[MCP2221_RESPONSE_ECHO_BYTE] =
		(mock_mode == MOCK_PROTOCOL_ERROR)
			? MCP2221_CMD_GET_GPIO_VALUES
			: (mock_mode == MOCK_SRAM_TIMEOUT_THEN_OK)
				? mock_last_cmd
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
	mock_sram_read_count = 0;
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

static void test_retry_transport_retries_timeout(void) {
	mcp2221_t dev = make_test_device();
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t response[MCP2221_PACKET_SIZE];

	reset_mock(MOCK_TIMEOUT_THEN_OK);

	assert(mcp2221_internal_send_cmd_retry_transport(&dev, &cmd, 1, response) ==
	       MCP2221_ERR_OK);
	assert(mock_write_count == 2);
	assert(mock_read_count == 2);
}

static void test_retry_transport_does_not_retry_command_failure(void) {
	mcp2221_t dev = make_test_device();
	uint8_t cmd = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t response[MCP2221_PACKET_SIZE];

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_internal_send_cmd_retry_transport(&dev, &cmd, 1, response) ==
	       MCP2221_ERR_COMMAND_FAILED);
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

static void test_i2c_rejects_oversized_read_chunk(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data[MCP2221_I2C_CHUNK_SIZE + 1] = {0};

	reset_mock(MOCK_I2C_GET_DATA_OVERSIZED_CHUNK);

	assert(mcp2221_i2c_read_ex(
		&dev,
		0x50,
		data,
		sizeof(data),
		MCP2221_I2C_KIND_NORMAL,
		100) == MCP2221_ERR_PROTOCOL);
	assert(dev.i2c_dirty == 1);
}

static void test_i2c_rejects_public_argument_boundaries(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data = 0;

	reset_mock(0);

	assert(mcp2221_i2c_write_ex(
		&dev, 0x80, &data, 1, MCP2221_I2C_KIND_NORMAL, 100) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_i2c_read_ex(
		&dev, 0x80, &data, 1, MCP2221_I2C_KIND_NORMAL, 100) ==
	       MCP2221_ERR_INVALID);

	assert(mcp2221_i2c_write_ex(
		&dev, 0x50, &data, 0, MCP2221_I2C_KIND_NORMAL, 100) ==
	       MCP2221_ERR_INVALID);
	assert(mcp2221_i2c_read_ex(
		&dev, 0x50, &data, 0, MCP2221_I2C_KIND_NORMAL, 100) ==
	       MCP2221_ERR_INVALID);

	assert(mcp2221_i2c_write_ex(
		&dev,
		0x50,
		&data,
		(size_t)MCP2221_I2C_TRANSFER_MAX + 1u,
		MCP2221_I2C_KIND_NORMAL,
		100) == MCP2221_ERR_INVALID);
	assert(mcp2221_i2c_read_ex(
		&dev,
		0x50,
		&data,
		(size_t)MCP2221_I2C_TRANSFER_MAX + 1u,
		MCP2221_I2C_KIND_NORMAL,
		100) == MCP2221_ERR_INVALID);

	assert(mcp2221_i2c_set_speed(&dev, 0) == MCP2221_ERR_INVALID);
	assert(mcp2221_i2c_set_speed(
		&dev, MCP2221_I2C_SPEED_MAX_HZ + 1u) == MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

static void test_i2c_accepts_maximum_documented_speed(void) {
	mcp2221_t dev = make_test_device();

	reset_mock(MOCK_I2C_SPEED_OK);

	assert(mcp2221_i2c_set_speed(
		&dev, MCP2221_I2C_SPEED_MAX_HZ) == MCP2221_ERR_OK);
	assert(mock_write_count == 1);
	assert(mock_read_count == 1);
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

static void test_flash_read_info_preserves_timeout(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_flash_info_t info;

	reset_mock(MOCK_READ_TIMEOUT);

	assert(mcp2221_flash_read_info(&dev, &info) == MCP2221_ERR_TIMEOUT);
}

static void test_flash_read_info_preserves_protocol_error(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_flash_info_t info;

	reset_mock(MOCK_PROTOCOL_ERROR);

	assert(mcp2221_flash_read_info(&dev, &info) == MCP2221_ERR_PROTOCOL);
}

static void test_flash_read_info_maps_command_failure(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_flash_info_t info;

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_flash_read_info(&dev, &info) == MCP2221_ERR_FLASH_READ);
}

static void test_flash_save_config_preserves_timeout(void) {
	mcp2221_t dev = make_test_device();

	reset_mock(MOCK_READ_TIMEOUT);

	assert(mcp2221_flash_save_config(&dev) == MCP2221_ERR_TIMEOUT);
}

static void test_flash_save_config_preserves_protocol_error(void) {
	mcp2221_t dev = make_test_device();

	reset_mock(MOCK_PROTOCOL_ERROR);

	assert(mcp2221_flash_save_config(&dev) == MCP2221_ERR_PROTOCOL);
}

static void test_flash_save_config_maps_command_failure(void) {
	mcp2221_t dev = make_test_device();

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_flash_save_config(&dev) == MCP2221_ERR_FLASH_READ);
}

static void test_flash_save_config_retries_sram_timeout(void) {
	mcp2221_t dev = make_test_device();

	reset_mock(MOCK_SRAM_TIMEOUT_THEN_OK);

	assert(mcp2221_flash_save_config(&dev) == MCP2221_ERR_OK);
	assert(mock_sram_read_count >= 2);
}

static void test_usb_get_remote_wakeup_preserves_timeout(void) {
	mcp2221_t dev = make_test_device();
	int enabled = 0;

	reset_mock(MOCK_READ_TIMEOUT);

	assert(mcp2221_usb_get_remote_wakeup(&dev, &enabled) ==
	       MCP2221_ERR_TIMEOUT);
}

static void test_usb_get_self_powered_preserves_protocol_error(void) {
	mcp2221_t dev = make_test_device();
	int self_powered = 0;

	reset_mock(MOCK_PROTOCOL_ERROR);

	assert(mcp2221_usb_get_self_powered(&dev, &self_powered) ==
	       MCP2221_ERR_PROTOCOL);
}

static void test_usb_get_requested_current_maps_flash_command_failure(void) {
	mcp2221_t dev = make_test_device();
	unsigned ma = 0;

	reset_mock(MOCK_FLASH_COMMAND_FAILURE);

	assert(mcp2221_usb_get_requested_current(&dev, &ma) ==
	       MCP2221_ERR_FLASH_READ);
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


static void test_gpio_write_rejects_out_of_contract_values(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_gpio_write_t wr = {
		.gp0 = MCP2221_GPIO_KEEP,
		.gp1 = 0,
		.gp2 = 1,
		.gp3 = MCP2221_GPIO_KEEP
	};

	reset_mock(0);

	wr.gp0 = -2;
	assert(mcp2221_gpio_write(&dev, &wr) == MCP2221_ERR_INVALID);

	wr.gp0 = MCP2221_GPIO_KEEP;
	wr.gp1 = 2;
	assert(mcp2221_gpio_write(&dev, &wr) == MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

static void test_pin_functions_rejects_non_boolean_outputs(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_pin_functions_t cfg = {
		.gp = {
			MCP2221_PIN_FUNC_KEEP,
			MCP2221_PIN_FUNC_KEEP,
			MCP2221_PIN_FUNC_KEEP,
			MCP2221_PIN_FUNC_KEEP
		},
		.out = {0, 0, 0, 0}
	};

	reset_mock(0);

	cfg.out[0] = 2;
	assert(mcp2221_pin_set_functions(&dev, &cfg) == MCP2221_ERR_INVALID);

	cfg.out[0] = 0;
	cfg.out[3] = -1;
	assert(mcp2221_pin_set_functions(&dev, &cfg) == MCP2221_ERR_INVALID);

	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}


static mcp2221_sram_config_t make_keep_sram_config(void) {
	mcp2221_sram_config_t cfg;

	for (int i = 0; i < 4; i++) {
		cfg.gp[i].value = MCP2221_CONFIG_KEEP;
		cfg.gp[i].direction = MCP2221_CONFIG_KEEP;
		cfg.gp[i].function = MCP2221_CONFIG_KEEP;
	}

	cfg.int_cfg.pos_edge = MCP2221_CONFIG_KEEP;
	cfg.int_cfg.neg_edge = MCP2221_CONFIG_KEEP;
	cfg.int_cfg.clear_flag = MCP2221_CONFIG_KEEP;

	cfg.adc_cfg.vrm = MCP2221_CONFIG_KEEP;
	cfg.adc_cfg.ref_src = MCP2221_CONFIG_KEEP;

	cfg.dac_ref.vrm = MCP2221_CONFIG_KEEP;
	cfg.dac_ref.ref_src = MCP2221_CONFIG_KEEP;

	cfg.dac_val.value = MCP2221_CONFIG_KEEP;

	cfg.clk_cfg.duty = MCP2221_CONFIG_KEEP;
	cfg.clk_cfg.div = MCP2221_CONFIG_KEEP;

	return cfg;
}

static void assert_sram_invalid_without_usb(
	mcp2221_t *dev,
	const mcp2221_sram_config_t *cfg) {
	reset_mock(0);

	assert(mcp2221_sram_config(dev, cfg) == MCP2221_ERR_INVALID);
	assert(mock_write_count == 0);
	assert(mock_read_count == 0);
}

static void test_sram_rejects_invalid_gpio_fields(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_sram_config_t cfg = make_keep_sram_config();

	cfg.gp[0].value = 2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.gp[1].direction = -2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.gp[0].function = MCP2221_GPIO_FUNC_ALT_1;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.gp[2].function = MCP2221_GPIO_FUNC_ALT_2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.gp[1].function = 7;
	assert_sram_invalid_without_usb(&dev, &cfg);
}

static void test_sram_rejects_invalid_interrupt_fields(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_sram_config_t cfg = make_keep_sram_config();

	cfg.int_cfg.pos_edge = 2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.int_cfg.neg_edge = -2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.int_cfg.clear_flag = 3;
	assert_sram_invalid_without_usb(&dev, &cfg);
}

static void test_sram_rejects_invalid_reference_fields(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_sram_config_t cfg = make_keep_sram_config();

	cfg.adc_cfg.ref_src = 2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.adc_cfg.vrm = 1;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.dac_ref.ref_src = -2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.dac_ref.vrm = 7;
	assert_sram_invalid_without_usb(&dev, &cfg);
}

static void test_sram_rejects_invalid_dac_value(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_sram_config_t cfg = make_keep_sram_config();

	cfg.dac_val.value = -2;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.dac_val.value = 32;
	assert_sram_invalid_without_usb(&dev, &cfg);
}

static void test_sram_rejects_invalid_clock_fields(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_sram_config_t cfg = make_keep_sram_config();

	cfg.clk_cfg.duty = 1;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.clk_cfg.div = 0;
	assert_sram_invalid_without_usb(&dev, &cfg);

	cfg = make_keep_sram_config();
	cfg.clk_cfg.div = 8;
	assert_sram_invalid_without_usb(&dev, &cfg);
}

static void test_open_rejects_null_output_pointer(void) {
	reset_mock(0);

	assert(mcp2221_open(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		500,
		3,
		0,
		0,
		NULL) == MCP2221_ERR_INVALID);

	assert(mcp2221_open_simple(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		100000,
		NULL) == MCP2221_ERR_INVALID);
}

static void test_open_propagates_no_memory(void) {
	mcp2221_t *dev = (mcp2221_t *)(uintptr_t)1;

	reset_mock(MOCK_OPEN_INIT_NO_MEMORY);

	assert(mcp2221_open(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		500,
		3,
		0,
		0,
		&dev) == MCP2221_ERR_NO_MEMORY);
	assert(dev == NULL);
}

static void test_open_propagates_usb_init_failure(void) {
	mcp2221_t *dev = (mcp2221_t *)(uintptr_t)1;

	reset_mock(MOCK_OPEN_INIT_FAILURE);

	assert(mcp2221_open(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		500,
		3,
		0,
		0,
		&dev) == MCP2221_ERR_USB_INIT);
	assert(dev == NULL);
}

static void test_open_propagates_not_found(void) {
	mcp2221_t *dev = (mcp2221_t *)(uintptr_t)1;

	reset_mock(MOCK_OPEN_NOT_FOUND);

	assert(mcp2221_open(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		500,
		3,
		0,
		0,
		&dev) == MCP2221_ERR_NOT_FOUND);
	assert(dev == NULL);
}

static void test_open_simple_propagates_not_found(void) {
	mcp2221_t *dev = (mcp2221_t *)(uintptr_t)1;

	reset_mock(MOCK_OPEN_NOT_FOUND);

	assert(mcp2221_open_simple(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		100000,
		&dev) == MCP2221_ERR_NOT_FOUND);
	assert(dev == NULL);
}

static mcp2221_i2c_slave_t make_test_slave(mcp2221_t *dev, int reg_bytes) {
	mcp2221_i2c_slave_t slave = {
		.mcp = dev,
		.addr = 0x50,
		.reg_bytes = reg_bytes,
		.reg_byteorder = MCP2221_I2C_BYTE_ORDER_BIG
	};
	return slave;
}

static void test_i2c_slave_read_register_rejects_out_of_range_register(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data;

	for (int reg_bytes = 1; reg_bytes <= 3; ++reg_bytes) {
		mcp2221_i2c_slave_t slave = make_test_slave(&dev, reg_bytes);
		uint32_t first_invalid = 1u << (8 * reg_bytes);

		reset_mock(0);
		assert(mcp2221_i2c_slave_read_register(
			&slave,
			first_invalid,
			&data,
			1,
			0,
			MCP2221_I2C_BYTE_ORDER_DEFAULT) == MCP2221_ERR_INVALID);
		assert(mock_write_count == 0);
		assert(mock_read_count == 0);
	}
}

static void test_i2c_slave_write_register_rejects_out_of_range_register(void) {
	mcp2221_t dev = make_test_device();
	uint8_t data = 0;

	for (int reg_bytes = 1; reg_bytes <= 3; ++reg_bytes) {
		mcp2221_i2c_slave_t slave = make_test_slave(&dev, reg_bytes);
		uint32_t first_invalid = 1u << (8 * reg_bytes);

		reset_mock(0);
		assert(mcp2221_i2c_slave_write_register(
			&slave,
			first_invalid,
			&data,
			1,
			0,
			MCP2221_I2C_BYTE_ORDER_DEFAULT) == MCP2221_ERR_INVALID);
		assert(mock_write_count == 0);
		assert(mock_read_count == 0);
	}
}

static void test_i2c_slave_init_invalidates_context_on_validation_failure(void);
static void test_i2c_slave_init_invalidates_context_on_speed_failure(void);

int main(void) {
	test_public_send_is_single_shot();
	test_retry_safe_retries_timeout();
	test_retry_safe_does_not_retry_protocol_error();
	test_retry_transport_retries_timeout();
	test_retry_transport_does_not_retry_command_failure();
	test_i2c_command_failure_maps_nack();
	test_i2c_address_timeout_maps_nack();
	test_i2c_command_failure_maps_unknown_state();
	test_i2c_rejects_oversized_read_chunk();
	test_i2c_rejects_public_argument_boundaries();
	test_i2c_accepts_maximum_documented_speed();
	test_i2c_slave_init_invalidates_context_on_validation_failure();
	test_i2c_slave_init_invalidates_context_on_speed_failure();
	test_i2c_slave_read_register_rejects_out_of_range_register();
	test_i2c_slave_write_register_rejects_out_of_range_register();
	test_flash_read_command_failure_maps_flash_read();
	test_flash_write_command_failure_maps_flash_write();
	test_flash_password_command_failure_maps_flash_password();
	test_flash_read_info_preserves_timeout();
	test_flash_read_info_preserves_protocol_error();
	test_flash_read_info_maps_command_failure();
	test_flash_save_config_preserves_timeout();
	test_flash_save_config_preserves_protocol_error();
	test_flash_save_config_maps_command_failure();
	test_flash_save_config_retries_sram_timeout();
	test_usb_get_remote_wakeup_preserves_timeout();
	test_usb_get_self_powered_preserves_protocol_error();
	test_usb_get_requested_current_maps_flash_command_failure();
	test_flash_rejects_null_arguments();
	test_flash_settings_rejects_null_arguments();
	test_gpio_poll_rejects_null_arguments();
	test_smbus_rejects_invalid_context_and_pointers();
	test_gpio_write_rejects_out_of_contract_values();
	test_pin_functions_rejects_non_boolean_outputs();
	test_sram_rejects_invalid_gpio_fields();
	test_sram_rejects_invalid_interrupt_fields();
	test_sram_rejects_invalid_reference_fields();
	test_sram_rejects_invalid_dac_value();
	test_sram_rejects_invalid_clock_fields();
	test_open_rejects_null_output_pointer();
	test_open_propagates_no_memory();
	test_open_propagates_usb_init_failure();
	test_open_propagates_not_found();
	test_open_simple_propagates_not_found();
	return 0;
}
static void assert_slave_context_invalid(const mcp2221_i2c_slave_t *slave) {
	assert(slave->mcp == NULL);
}

static void test_i2c_slave_init_invalidates_context_on_validation_failure(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_i2c_slave_t slave = {
		.mcp = &dev,
		.addr = 0x50,
		.reg_bytes = 4,
		.reg_byteorder = MCP2221_I2C_BYTE_ORDER_LITTLE
	};

	reset_mock(0);

	assert(mcp2221_i2c_slave_init(
		&slave,
		&dev,
		0x50,
		1,
		100000,
		5,
		MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_INVALID);
	assert_slave_context_invalid(&slave);
}

static void test_i2c_slave_init_invalidates_context_on_speed_failure(void) {
	mcp2221_t dev = make_test_device();
	mcp2221_i2c_slave_t slave = {
		.mcp = &dev,
		.addr = 0x50,
		.reg_bytes = 4,
		.reg_byteorder = MCP2221_I2C_BYTE_ORDER_LITTLE
	};

	reset_mock(MOCK_READ_TIMEOUT);

	assert(mcp2221_i2c_slave_init(
		&slave,
		&dev,
		0x50,
		1,
		100000,
		1,
		MCP2221_I2C_BYTE_ORDER_BIG) == MCP2221_ERR_TIMEOUT);
	assert_slave_context_invalid(&slave);
}

