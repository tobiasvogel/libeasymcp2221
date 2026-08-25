#include <assert.h>
#include <libusb.h>
#include <stdint.h>
#include <string.h>

#include "fake_libusb.h"
#include "mcp2221.h"
#include "mcp2221_analog.h"
#include "mcp2221_flash_info.h"
#include "mcp2221_gpio.h"
#include "mcp2221_gpio_poll.h"
#include "mcp2221_internal.h"
#include "mcp2221_internal_constants.h"
#include "mcp2221_sram.h"

enum {
	MCP2221_TEST_FLASH_DESCRIPTOR_TYPE_BYTE = 3,
	MCP2221_TEST_I2C_GET_DATA_COUNT_BYTE = 3,
	MCP2221_TEST_GPIO_VALUE_LOW = 0x00u,
	MCP2221_TEST_GPIO_VALUE_HIGH = 0x01u,
	MCP2221_TEST_GPIO_VALUE_NOT_GPIO = 0xEEu,
	MCP2221_TEST_GPIO_VALUE_INVALID = 0x02u,
	MCP2221_TEST_SRAM_RESPONSE_ADC_REF_SHIFT = 2,
	MCP2221_TEST_SRAM_RESPONSE_NEG_EDGE_ENABLED = (1u << 6),
	MCP2221_TEST_COMMAND_FAILURE = 0x01u,
};

static mcp2221_t *open_test_device(void) {
	fake_libusb_reset();
	fake_libusb_configure_device(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		"TESTSERIAL");

	mcp2221_t *dev = NULL;
	assert(mcp2221_open(
		MCP2221_DEV_DEFAULT_VID,
		MCP2221_DEV_DEFAULT_PID,
		0,
		NULL,
		10,
		0,
		0,
		0,
		&dev) == MCP2221_ERR_OK);
	assert(dev != NULL);
	return dev;
}

static void queue_success_response(
	const uint8_t *command, size_t command_len,
	const uint8_t response[MCP2221_PACKET_SIZE]) {
	fake_libusb_expect_write(command, command_len);
	fake_libusb_queue_read(response);
}

static void queue_flash_read(
	uint8_t section, uint8_t structure_length,
	const uint8_t *payload, size_t payload_len) {
	assert(payload_len <= MCP2221_PACKET_SIZE - MCP2221_FLASH_OFFSET_READ);

	uint8_t command[2] = {
		MCP2221_CMD_READ_FLASH_DATA,
		section,
	};
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = MCP2221_CMD_READ_FLASH_DATA;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_FLASH_RESPONSE_STRUCTURE_LENGTH_BYTE] = structure_length;
	if (section == MCP2221_FLASH_DATA_USB_MANUFACTURER ||
	    section == MCP2221_FLASH_DATA_USB_PRODUCT ||
	    section == MCP2221_FLASH_DATA_USB_SERIALNUM)
		response[MCP2221_TEST_FLASH_DESCRIPTOR_TYPE_BYTE] = LIBUSB_DT_STRING;
	if (payload_len != 0)
		memcpy(&response[MCP2221_FLASH_OFFSET_READ], payload, payload_len);

	queue_success_response(command, sizeof(command), response);
}

static void queue_flash_write(
	uint8_t section, const uint8_t payload[60]) {
	uint8_t command[MCP2221_PACKET_SIZE] = {0};
	command[0] = MCP2221_CMD_WRITE_FLASH_DATA;
	command[1] = section;
	memcpy(&command[MCP2221_FLASH_OFFSET_WRITE], payload, 60);

	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = MCP2221_CMD_WRITE_FLASH_DATA;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(command, sizeof(command), response);
}

static void test_open_discovers_hid_and_send_cmd_succeeds(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t expected[MCP2221_PACKET_SIZE] = {0};
	expected[MCP2221_RESPONSE_ECHO_BYTE] = command;
	expected[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	expected[2] = 0xA5;
	queue_success_response(&command, 1, expected);

	uint8_t actual[MCP2221_PACKET_SIZE] = {0};
	assert(mcp2221_send_cmd(dev, &command, 1, actual) == MCP2221_ERR_OK);
	assert(memcmp(actual, expected, sizeof(actual)) == 0);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_send_cmd_maps_read_timeout(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	fake_libusb_expect_write(&command, 1);
	fake_libusb_queue_read_result(NULL, 0, LIBUSB_ERROR_TIMEOUT, 0);

	assert(mcp2221_send_cmd(dev, &command, 1, NULL) == MCP2221_ERR_TIMEOUT);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_send_cmd_maps_write_timeout(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	fake_libusb_expect_write_result(
		&command, 1, LIBUSB_ERROR_TIMEOUT, 0);

	assert(mcp2221_send_cmd(dev, &command, 1, NULL) == MCP2221_ERR_TIMEOUT);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_send_cmd_preserves_non_timeout_read_error(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	fake_libusb_expect_write(&command, 1);
	fake_libusb_queue_read_result(NULL, 0, LIBUSB_ERROR_NO_DEVICE, 0);

	assert(mcp2221_send_cmd(dev, &command, 1, NULL) == MCP2221_ERR_USB);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_send_cmd_rejects_short_read(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	fake_libusb_expect_write(&command, 1);
	fake_libusb_queue_read_result(
		response, MCP2221_PACKET_SIZE - 1,
		LIBUSB_SUCCESS, MCP2221_PACKET_SIZE - 1);

	assert(mcp2221_send_cmd(dev, &command, 1, NULL) == MCP2221_ERR_USB);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void queue_i2c_status_timeout(void) {
	uint8_t command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	fake_libusb_expect_write(&command, 1);
	fake_libusb_queue_read_result(NULL, 0, LIBUSB_ERROR_TIMEOUT, 0);
}

static void test_i2c_transfers_propagate_preflight_status_timeout(void) {
	uint8_t data = 0x5Au;

	mcp2221_t *dev = open_test_device();
	queue_i2c_status_timeout();
	assert(mcp2221_i2c_write_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_TIMEOUT);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);

	dev = open_test_device();
	data = 0xA5u;
	queue_i2c_status_timeout();
	assert(mcp2221_i2c_read_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_TIMEOUT);
	assert(data == 0xA5u);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void queue_i2c_status_response(
	uint8_t scl, uint8_t sda, int confused) {
	uint8_t command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_I2C_POLL_RESP_SCL] = scl;
	response[MCP2221_I2C_POLL_RESP_SDA] = sda;
	if (confused)
		response[MCP2221_I2C_POLL_RESP_UNDOCUMENTED_18] =
			MCP2221_I2C_CONFUSED_MARKER;
	queue_success_response(&command, 1, response);
}

static void queue_i2c_preflight_release_line_error(
	uint8_t scl, uint8_t sda) {
	/* Transfer preflight notices external SDA activity and requests release. */
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 1);

	/* release(): initial status says the engine has not been initialized. */
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 0);

	/* release(): final status diagnoses the stuck bus line. */
	queue_i2c_status_response(scl, sda, 0);
}

static void test_i2c_transfers_propagate_preflight_release_line_errors(void) {
	uint8_t data = 0x5Au;

	mcp2221_t *dev = open_test_device();
	queue_i2c_preflight_release_line_error(
		MCP2221_I2C_LINE_LOW, MCP2221_I2C_LINE_HIGH);
	assert(mcp2221_i2c_write_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_LOW_SCL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);

	dev = open_test_device();
	data = 0xA5u;
	queue_i2c_preflight_release_line_error(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_LOW);
	assert(mcp2221_i2c_read_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_LOW_SDA);
	assert(data == 0xA5u);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void queue_i2c_command_failure(
	const uint8_t *command, size_t command_len, uint8_t internal_status) {
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command[0];
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_TEST_COMMAND_FAILURE;
	response[MCP2221_I2C_INTERNAL_STATUS_BYTE] = internal_status;
	queue_success_response(command, command_len, response);
}

static void test_i2c_transfers_propagate_runtime_release_line_errors(void) {
	uint8_t data = 0x5Au;

	mcp2221_t *dev = open_test_device();
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 0);

	uint8_t write_command[5] = {
		MCP2221_CMD_I2C_WRITE_DATA,
		1, 0,
		(uint8_t)(0x50u << 1),
		data,
	};
	queue_i2c_command_failure(
		write_command, sizeof(write_command),
		MCP2221_I2C_ST_WRADDRL_NACK_STOP);
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 0);
	queue_i2c_status_response(
		MCP2221_I2C_LINE_LOW, MCP2221_I2C_LINE_HIGH, 0);

	assert(mcp2221_i2c_write_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_LOW_SCL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);

	dev = open_test_device();
	data = 0xA5u;
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 0);

	uint8_t read_command[4] = {
		MCP2221_CMD_I2C_READ_DATA,
		1, 0,
		(uint8_t)((0x50u << 1) + 1u),
	};
	queue_i2c_command_failure(
		read_command, sizeof(read_command),
		MCP2221_I2C_ST_WRADDRL_NACK_STOP);
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 0);
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_LOW, 0);

	assert(mcp2221_i2c_read_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_LOW_SDA);
	assert(data == 0xA5u);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_i2c_speed_propagates_release_line_error(void) {
	mcp2221_t *dev = open_test_device();

	const uint32_t speed_hz = 100000u;
	uint8_t speed_command[5] = {
		MCP2221_CMD_POLL_STATUS_SET_PARAMETERS,
		0,
		0,
		MCP2221_I2C_CMD_SET_BUS_SPEED,
		(uint8_t)(
			MCP2221_I2C_BASE_CLOCK_HZ / speed_hz -
			MCP2221_I2C_CLOCK_DIVIDER_OFFSET),
	};

	/* A failed speed command marks the bus dirty through the public path. */
	fake_libusb_expect_write(speed_command, sizeof(speed_command));
	fake_libusb_queue_read_result(NULL, 0, LIBUSB_ERROR_TIMEOUT, 0);
	assert(mcp2221_i2c_set_speed(dev, speed_hz) == MCP2221_ERR_TIMEOUT);

	uint8_t speed_response[MCP2221_PACKET_SIZE] = {0};
	speed_response[MCP2221_RESPONSE_ECHO_BYTE] =
		MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	speed_response[MCP2221_RESPONSE_STATUS_BYTE] =
		MCP2221_RESPONSE_RESULT_OK;
	speed_response[MCP2221_I2C_POLL_RESP_NEWSPEED_STATUS] =
		MCP2221_I2C_NEWSPEED_NOT_SET;
	queue_success_response(
		speed_command, sizeof(speed_command), speed_response);

	/* release(): initial status says the engine has not been initialized. */
	queue_i2c_status_response(
		MCP2221_I2C_LINE_HIGH, MCP2221_I2C_LINE_HIGH, 0);

	/* release(): final status diagnoses a stuck SCL line. */
	queue_i2c_status_response(
		MCP2221_I2C_LINE_LOW, MCP2221_I2C_LINE_HIGH, 0);

	assert(mcp2221_i2c_set_speed(dev, speed_hz) == MCP2221_ERR_LOW_SCL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_i2c_speed_rejects_invalid_response_status(void) {
	mcp2221_t *dev = open_test_device();

	const uint32_t speed_hz = 100000u;
	uint8_t speed_command[5] = {
		MCP2221_CMD_POLL_STATUS_SET_PARAMETERS,
		0,
		0,
		MCP2221_I2C_CMD_SET_BUS_SPEED,
		(uint8_t)(
			MCP2221_I2C_BASE_CLOCK_HZ / speed_hz -
			MCP2221_I2C_CLOCK_DIVIDER_OFFSET),
	};
	uint8_t speed_response[MCP2221_PACKET_SIZE] = {0};
	speed_response[MCP2221_RESPONSE_ECHO_BYTE] =
		MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	speed_response[MCP2221_RESPONSE_STATUS_BYTE] =
		MCP2221_RESPONSE_RESULT_OK;
	speed_response[MCP2221_I2C_POLL_RESP_NEWSPEED_STATUS] =
		MCP2221_I2C_NEWSPEED_NOT_SET + 1u;
	queue_success_response(
		speed_command, sizeof(speed_command), speed_response);

	assert(mcp2221_i2c_set_speed(dev, speed_hz) == MCP2221_ERR_PROTOCOL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_i2c_status_rejects_invalid_line_levels(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_I2C_POLL_RESP_SCL] = MCP2221_I2C_LINE_HIGH + 1u;
	response[MCP2221_I2C_POLL_RESP_SDA] = MCP2221_I2C_LINE_HIGH;
	queue_success_response(&command, 1, response);

	mcp2221_i2c_status_t status;
	memset(&status, 0xA5, sizeof(status));
	mcp2221_i2c_status_t before = status;

	assert(mcp2221_i2c_status(dev, &status) == MCP2221_ERR_PROTOCOL);
	assert(memcmp(&status, &before, sizeof(status)) == 0);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void queue_malformed_gpio_response(void) {
	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_GPIO_GET_RESP_GP0_VALUE] = MCP2221_TEST_GPIO_VALUE_LOW;
	response[MCP2221_GPIO_GET_RESP_GP1_VALUE] = MCP2221_TEST_GPIO_VALUE_HIGH;
	response[MCP2221_GPIO_GET_RESP_GP2_VALUE] = MCP2221_TEST_GPIO_VALUE_INVALID;
	response[MCP2221_GPIO_GET_RESP_GP3_VALUE] = MCP2221_TEST_GPIO_VALUE_NOT_GPIO;
	queue_success_response(&command, 1, response);
}

static void test_gpio_reads_reject_malformed_values(void) {
	mcp2221_t *dev = open_test_device();

	int values[4] = {7, 7, 7, 7};
	queue_malformed_gpio_response();
	assert(mcp2221_gpio_read(dev, values) == MCP2221_ERR_PROTOCOL);
	for (int i = 0; i < 4; i++)
		assert(values[i] == 7);

	uint8_t valid_mask = 0xA5;
	queue_malformed_gpio_response();
	assert(mcp2221_gpio_read_mask(dev, values, &valid_mask) == MCP2221_ERR_PROTOCOL);
	for (int i = 0; i < 4; i++)
		assert(values[i] == 7);
	assert(valid_mask == 0xA5);

	mcp2221_gpio_poll_state_t poll_state;
	mcp2221_gpio_change_t changes[4] = {0};
	mcp2221_gpio_poll_init(&poll_state);
	queue_malformed_gpio_response();
	assert(mcp2221_gpio_poll(dev, &poll_state, changes) == MCP2221_ERR_PROTOCOL);
	assert(poll_state.initialized == 0);
	for (int i = 0; i < 4; i++)
		assert(poll_state.prev[i] == -2);

	uint16_t filter = MCP2221_GPIO_POLL_MASK_RISE(MCP2221_GPIO_GP0);
	mcp2221_gpio_event_t event = {0};
	queue_malformed_gpio_response();
	assert(mcp2221_gpio_poll_events(
		dev, &poll_state, &filter, &event, 1) == MCP2221_ERR_PROTOCOL);
	assert(poll_state.initialized == 0);
	assert(poll_state.filter_mask == 0);
	for (int i = 0; i < 4; i++)
		assert(poll_state.prev[i] == -2);

	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void queue_gpio_values(
	uint8_t gp0, uint8_t gp1, uint8_t gp2, uint8_t gp3) {
	uint8_t command = MCP2221_CMD_GET_GPIO_VALUES;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_GPIO_GET_RESP_GP0_VALUE] = gp0;
	response[MCP2221_GPIO_GET_RESP_GP1_VALUE] = gp1;
	response[MCP2221_GPIO_GET_RESP_GP2_VALUE] = gp2;
	response[MCP2221_GPIO_GET_RESP_GP3_VALUE] = gp3;
	queue_success_response(&command, 1, response);
}

static void test_gpio_poll_helpers_share_timestamp_state(void) {
	mcp2221_t *dev = open_test_device();
	mcp2221_gpio_poll_state_t poll_state;
	mcp2221_gpio_change_t changes[4] = {0};
	mcp2221_gpio_poll_init(&poll_state);

	queue_gpio_values(
		MCP2221_TEST_GPIO_VALUE_LOW,
		MCP2221_TEST_GPIO_VALUE_LOW,
		MCP2221_TEST_GPIO_VALUE_LOW,
		MCP2221_TEST_GPIO_VALUE_LOW);
	assert(mcp2221_gpio_poll(dev, &poll_state, changes) == MCP2221_ERR_OK);
	assert(poll_state.last_time > 0.0);
	double first_poll_time = poll_state.last_time;

	queue_gpio_values(
		MCP2221_TEST_GPIO_VALUE_HIGH,
		MCP2221_TEST_GPIO_VALUE_LOW,
		MCP2221_TEST_GPIO_VALUE_LOW,
		MCP2221_TEST_GPIO_VALUE_LOW);
	mcp2221_gpio_event_t event = {0};
	assert(mcp2221_gpio_poll_events(
		dev, &poll_state, NULL, &event, 1) == 1);
	assert(event.gpio == MCP2221_GPIO_GP0);
	assert(event.type == MCP2221_GPIO_EVENT_RISE);
	assert(event.last_time == first_poll_time);

	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_gpio_write_rejects_inconsistent_response(void) {
	mcp2221_t *dev = open_test_device();

	mcp2221_gpio_write_t wr = {
		.gp0 = 1,
		.gp1 = MCP2221_GPIO_KEEP,
		.gp2 = MCP2221_GPIO_KEEP,
		.gp3 = MCP2221_GPIO_KEEP,
	};

	uint8_t command[MCP2221_GPIO_SET_COMMAND_SIZE] = {0};
	command[0] = MCP2221_CMD_SET_GPIO_OUTPUT_VALUES;
	command[MCP2221_GPIO_SET_GP0_ALTER_BYTE] = MCP2221_GPIO_SET_ALTER_VALUE;
	command[MCP2221_GPIO_SET_GP0_VALUE_BYTE] = MCP2221_TEST_GPIO_VALUE_HIGH;

	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = MCP2221_CMD_SET_GPIO_OUTPUT_VALUES;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_GPIO_SET_GP0_ALTER_BYTE] = MCP2221_GPIO_RESPONSE_NOT_GPIO;
	response[MCP2221_GPIO_SET_GP0_VALUE_BYTE] = MCP2221_TEST_GPIO_VALUE_HIGH;
	queue_success_response(command, sizeof(command), response);

	uint8_t get_command = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t get_response[MCP2221_PACKET_SIZE] = {0};
	get_response[MCP2221_RESPONSE_ECHO_BYTE] = get_command;
	get_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(&get_command, 1, get_response);

	assert(mcp2221_gpio_write(dev, &wr) == MCP2221_ERR_PROTOCOL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_adc_read_raw_rejects_values_above_10_bits(void) {
	mcp2221_t *dev = open_test_device();

	uint16_t valid_adc = MCP2221_ADC_RAW_MAX;
	uint16_t invalid_adc = MCP2221_ADC_RAW_MAX + 1u;
	uint8_t command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_I2C_POLL_RESP_ADC_CH0_LSB] = (uint8_t)valid_adc;
	response[MCP2221_I2C_POLL_RESP_ADC_CH0_MSB] = (uint8_t)(valid_adc >> 8);
	response[MCP2221_I2C_POLL_RESP_ADC_CH1_LSB] = (uint8_t)invalid_adc;
	response[MCP2221_I2C_POLL_RESP_ADC_CH1_MSB] = (uint8_t)(invalid_adc >> 8);
	queue_success_response(&command, 1, response);

	uint16_t out[3] = {11u, 22u, 33u};
	assert(mcp2221_adc_read_raw(dev, out) == MCP2221_ERR_PROTOCOL);
	assert(out[0] == 11u);
	assert(out[1] == 22u);
	assert(out[2] == 33u);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_ioc_read_rejects_malformed_state(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = command;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_I2C_POLL_RESP_INT_FLAG] =
		MCP2221_INT_EDGE_STATE_ACTIVE + 1u;
	queue_success_response(&command, 1, response);

	uint8_t flag = 0xA5u;
	assert(mcp2221_ioc_read(dev, &flag) == MCP2221_ERR_PROTOCOL);
	assert(flag == 0xA5u);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_sram_interrupt_keep_does_not_reuse_adc_bits(void) {
	mcp2221_t *dev = open_test_device();

	const uint8_t adc_ref =
		MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_1024;
	uint8_t get_command = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t get_response[MCP2221_PACKET_SIZE] = {0};
	get_response[MCP2221_RESPONSE_ECHO_BYTE] = get_command;
	get_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	get_response[MCP2221_SRAM_RESPONSE_INT_ADC] =
		(uint8_t)(
			MCP2221_TEST_SRAM_RESPONSE_NEG_EDGE_ENABLED |
			(adc_ref << MCP2221_TEST_SRAM_RESPONSE_ADC_REF_SHIFT));

	/* First GET initializes the GPIO cache; the second is mcp2221_sram_config(). */
	queue_success_response(&get_command, 1, get_response);
	queue_success_response(&get_command, 1, get_response);

	mcp2221_sram_config_t cfg = {
		.gp = {
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		},
		.int_cfg = {1, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		.adc_cfg = {MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		.dac_ref = {MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		.dac_val = {MCP2221_CONFIG_KEEP},
		.clk_cfg = {MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
	};

	uint8_t set_command[12] = {
		MCP2221_CMD_SET_SRAM_SETTINGS,
		0,
		MCP2221_PRESERVE_CLK_OUTPUT,
		MCP2221_ALTER_DAC_REF | MCP2221_DAC_REF_VDD,
		MCP2221_ALTER_DAC_VALUE,
		MCP2221_ALTER_ADC_REF | adc_ref,
		MCP2221_ALTER_INT_CONF | MCP2221_INT_POS_EDGE_ENABLE,
		MCP2221_PRESERVE_GPIO_CONF,
		0, 0, 0, 0,
	};
	uint8_t set_response[MCP2221_PACKET_SIZE] = {0};
	set_response[MCP2221_RESPONSE_ECHO_BYTE] =
		MCP2221_CMD_SET_SRAM_SETTINGS;
	set_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(set_command, sizeof(set_command), set_response);

	assert(mcp2221_sram_config(dev, &cfg) == MCP2221_ERR_OK);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_sram_cache_tracks_gpio_when_vrm_reclaim_fails(void) {
	mcp2221_t *dev = open_test_device();

	const uint8_t adc_ref =
		MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_1024;
	const uint8_t old_gp[4] = {
		MCP2221_GPIO_DIR_IN | MCP2221_GPIO_FUNC_GPIO,
		MCP2221_GPIO_DIR_IN | MCP2221_GPIO_FUNC_GPIO,
		MCP2221_GPIO_FUNC_GPIO,
		MCP2221_GPIO_FUNC_GPIO,
	};
	const uint8_t new_gp0 =
		MCP2221_GPIO_OUT_VAL_1 | MCP2221_GPIO_FUNC_GPIO;

	uint8_t get_command = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t get_response[MCP2221_PACKET_SIZE] = {0};
	get_response[MCP2221_RESPONSE_ECHO_BYTE] = get_command;
	get_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	get_response[MCP2221_SRAM_RESPONSE_INT_ADC] =
		(uint8_t)(adc_ref << MCP2221_TEST_SRAM_RESPONSE_ADC_REF_SHIFT);
	get_response[MCP2221_SRAM_RESPONSE_GP0] = old_gp[0];
	get_response[MCP2221_SRAM_RESPONSE_GP1] = old_gp[1];
	get_response[MCP2221_SRAM_RESPONSE_GP2] = old_gp[2];
	get_response[MCP2221_SRAM_RESPONSE_GP3] = old_gp[3];

	/* First GET initializes the GPIO cache; the second reads current SRAM. */
	queue_success_response(&get_command, 1, get_response);
	queue_success_response(&get_command, 1, get_response);

	mcp2221_sram_config_t cfg = {
		.gp = {
			{1, MCP2221_DIR_OUTPUT, MCP2221_GPIO_FUNC_GPIO},
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
			{MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		},
		.int_cfg = {
			MCP2221_CONFIG_KEEP,
			MCP2221_CONFIG_KEEP,
			MCP2221_CONFIG_KEEP,
		},
		.adc_cfg = {MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		.dac_ref = {MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
		.dac_val = {MCP2221_CONFIG_KEEP},
		.clk_cfg = {MCP2221_CONFIG_KEEP, MCP2221_CONFIG_KEEP},
	};

	uint8_t gpio_command[12] = {
		MCP2221_CMD_SET_SRAM_SETTINGS,
		0,
		MCP2221_PRESERVE_CLK_OUTPUT,
		MCP2221_ALTER_DAC_REF |
			(MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_OFF),
		MCP2221_ALTER_DAC_VALUE,
		MCP2221_ALTER_ADC_REF |
			(MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_OFF),
		MCP2221_PRESERVE_INT_CONF,
		MCP2221_ALTER_GPIO_CONF,
		new_gp0,
		old_gp[1],
		old_gp[2],
		old_gp[3],
	};
	uint8_t gpio_response[MCP2221_PACKET_SIZE] = {0};
	gpio_response[MCP2221_RESPONSE_ECHO_BYTE] =
		MCP2221_CMD_SET_SRAM_SETTINGS;
	gpio_response[MCP2221_RESPONSE_STATUS_BYTE] =
		MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(
		gpio_command, sizeof(gpio_command), gpio_response);

	uint8_t reclaim_command[12] = {
		MCP2221_CMD_SET_SRAM_SETTINGS,
		0,
		MCP2221_PRESERVE_CLK_OUTPUT,
		MCP2221_ALTER_DAC_REF | MCP2221_DAC_REF_VDD,
		MCP2221_ALTER_DAC_VALUE,
		MCP2221_ALTER_ADC_REF | adc_ref,
		MCP2221_PRESERVE_INT_CONF,
		MCP2221_PRESERVE_GPIO_CONF,
		0, 0, 0, 0,
	};
	uint8_t reclaim_response[MCP2221_PACKET_SIZE] = {0};
	reclaim_response[MCP2221_RESPONSE_ECHO_BYTE] =
		MCP2221_CMD_SET_SRAM_SETTINGS;
	reclaim_response[MCP2221_RESPONSE_STATUS_BYTE] =
		MCP2221_TEST_COMMAND_FAILURE;
	queue_success_response(
		reclaim_command, sizeof(reclaim_command), reclaim_response);

	assert(mcp2221_sram_config(dev, &cfg) == MCP2221_ERR_COMMAND_FAILED);

	uint8_t cached[4] = {0};
	assert(mcp2221_internal_gpio_status_get(dev, cached) == MCP2221_ERR_OK);
	assert(cached[0] == new_gp0);
	assert(cached[1] == old_gp[1]);
	assert(cached[2] == old_gp[2]);
	assert(cached[3] == old_gp[3]);

	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_i2c_get_data_error_count_maps_i2c_error(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t status_command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t status_response[MCP2221_PACKET_SIZE] = {0};
	status_response[MCP2221_RESPONSE_ECHO_BYTE] = status_command;
	status_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(&status_command, 1, status_response);

	uint8_t read_command[4] = {
		MCP2221_CMD_I2C_READ_DATA,
		1,
		0,
		(uint8_t)((0x50u << 1) | 1u),
	};
	uint8_t read_response[MCP2221_PACKET_SIZE] = {0};
	read_response[MCP2221_RESPONSE_ECHO_BYTE] = MCP2221_CMD_I2C_READ_DATA;
	read_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(read_command, sizeof(read_command), read_response);

	uint8_t get_data_command = MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA;
	uint8_t get_data_response[MCP2221_PACKET_SIZE] = {0};
	get_data_response[MCP2221_RESPONSE_ECHO_BYTE] = get_data_command;
	get_data_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	get_data_response[MCP2221_I2C_INTERNAL_STATUS_BYTE] =
		MCP2221_I2C_ST_READDATA_WAITGET;
	get_data_response[MCP2221_TEST_I2C_GET_DATA_COUNT_BYTE] =
		MCP2221_I2C_GET_DATA_ERROR_COUNT;
	get_data_response[4] = 0xA5;
	queue_success_response(&get_data_command, 1, get_data_response);

	uint8_t data = 0x5A;
	assert(mcp2221_i2c_read_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_I2C);
	assert(data == 0x5A);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_i2c_rejects_chunk_larger_than_remaining_request(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t status_command = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t status_response[MCP2221_PACKET_SIZE] = {0};
	status_response[MCP2221_RESPONSE_ECHO_BYTE] = status_command;
	status_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(&status_command, 1, status_response);

	uint8_t read_command[4] = {
		MCP2221_CMD_I2C_READ_DATA,
		1,
		0,
		(uint8_t)((0x50u << 1) | 1u),
	};
	uint8_t read_response[MCP2221_PACKET_SIZE] = {0};
	read_response[MCP2221_RESPONSE_ECHO_BYTE] = MCP2221_CMD_I2C_READ_DATA;
	read_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	queue_success_response(read_command, sizeof(read_command), read_response);

	uint8_t get_data_command = MCP2221_CMD_I2C_READ_DATA_GET_I2C_DATA;
	uint8_t get_data_response[MCP2221_PACKET_SIZE] = {0};
	get_data_response[MCP2221_RESPONSE_ECHO_BYTE] = get_data_command;
	get_data_response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	get_data_response[MCP2221_I2C_INTERNAL_STATUS_BYTE] =
		MCP2221_I2C_ST_READDATA_WAITGET;
	get_data_response[MCP2221_TEST_I2C_GET_DATA_COUNT_BYTE] = 2;
	get_data_response[4] = 0xA5;
	get_data_response[5] = 0x5A;
	queue_success_response(&get_data_command, 1, get_data_response);

	uint8_t data = 0x3C;
	assert(mcp2221_i2c_read_ex(
		dev, 0x50, &data, 1,
		MCP2221_I2C_KIND_NORMAL, 100) == MCP2221_ERR_PROTOCOL);
	assert(data == 0x3C);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_flash_save_config_uses_sram_payload_offsets(void) {
	mcp2221_t *dev = open_test_device();

	uint8_t flash_chip[60] = {0};
	uint8_t flash_gp[60] = {0};
	flash_chip[MCP2221_FLASH_CHIP_SETTINGS_USBPWR] = 0xA0u;
	flash_chip[MCP2221_FLASH_CHIP_SETTINGS_USBMA] = 50u;

	queue_flash_read(
		MCP2221_FLASH_DATA_CHIP_SETTINGS,
		0, flash_chip, sizeof(flash_chip));
	queue_flash_read(
		MCP2221_FLASH_DATA_GP_SETTINGS,
		0, flash_gp, sizeof(flash_gp));

	uint8_t get_command = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t sram[MCP2221_PACKET_SIZE] = {0};
	sram[MCP2221_RESPONSE_ECHO_BYTE] = get_command;
	sram[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;

	uint8_t *settings = &sram[MCP2221_SRAM_RESPONSE_SETTINGS_OFFSET];
	settings[MCP2221_SRAM_CHIP_SETTINGS_CDCSEC] = 0xA4u;
	settings[MCP2221_SRAM_CHIP_SETTINGS_CLOCK] = 0x1Du;
	settings[MCP2221_SRAM_CHIP_SETTINGS_DAC] = 0x6Bu;
	settings[MCP2221_SRAM_CHIP_SETTINGS_INT_ADC] = 0x54u;
	settings[MCP2221_SRAM_CHIP_SETTINGS_LVID] =
		(uint8_t)MCP2221_DEV_DEFAULT_VID;
	settings[MCP2221_SRAM_CHIP_SETTINGS_HVID] =
		(uint8_t)(MCP2221_DEV_DEFAULT_VID >> 8);
	settings[MCP2221_SRAM_CHIP_SETTINGS_LPID] =
		(uint8_t)MCP2221_DEV_DEFAULT_PID;
	settings[MCP2221_SRAM_CHIP_SETTINGS_HPID] =
		(uint8_t)(MCP2221_DEV_DEFAULT_PID >> 8);
	for (int i = 0; i < 8; i++)
		settings[MCP2221_SRAM_CHIP_SETTINGS_PWD1 + i] =
			(uint8_t)(0x11u + (unsigned)i);

	sram[MCP2221_SRAM_RESPONSE_GP0] = 0x10u;
	sram[MCP2221_SRAM_RESPONSE_GP1] = 0x18u;
	sram[MCP2221_SRAM_RESPONSE_GP2] = 0x00u;
	sram[MCP2221_SRAM_RESPONSE_GP3] = 0x08u;

	/* save_config() reads SRAM once, then once more to initialize the GP cache. */
	queue_success_response(&get_command, 1, sram);
	queue_success_response(&get_command, 1, sram);

	uint8_t expected_chip[60];
	memcpy(expected_chip, flash_chip, sizeof(expected_chip));
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_CDCSEC] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_CDCSEC];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_CLOCK] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_CLOCK];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_DAC] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_DAC];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_INT_ADC] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_INT_ADC];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_LVID] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_LVID];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_HVID] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_HVID];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_LPID] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_LPID];
	expected_chip[MCP2221_FLASH_CHIP_SETTINGS_HPID] =
		settings[MCP2221_SRAM_CHIP_SETTINGS_HPID];
	for (int i = 0; i < 8; i++)
		expected_chip[MCP2221_FLASH_CHIP_SETTINGS_PWD1 + i] =
			settings[MCP2221_SRAM_CHIP_SETTINGS_PWD1 + i];

	uint8_t expected_gp[60];
	memcpy(expected_gp, flash_gp, sizeof(expected_gp));
	expected_gp[MCP2221_FLASH_GP_SETTINGS_GP0] =
		sram[MCP2221_SRAM_RESPONSE_GP0];
	expected_gp[MCP2221_FLASH_GP_SETTINGS_GP1] =
		sram[MCP2221_SRAM_RESPONSE_GP1];
	expected_gp[MCP2221_FLASH_GP_SETTINGS_GP2] =
		sram[MCP2221_SRAM_RESPONSE_GP2];
	expected_gp[MCP2221_FLASH_GP_SETTINGS_GP3] =
		sram[MCP2221_SRAM_RESPONSE_GP3];

	queue_flash_write(MCP2221_FLASH_DATA_CHIP_SETTINGS, expected_chip);
	queue_flash_write(MCP2221_FLASH_DATA_GP_SETTINGS, expected_gp);

	assert(mcp2221_flash_save_config(dev) == MCP2221_ERR_OK);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_flash_info_uses_response_structure_lengths(void) {
	mcp2221_t *dev = open_test_device();

	queue_flash_read(MCP2221_FLASH_DATA_CHIP_SETTINGS, 0, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_GP_SETTINGS, 0, NULL, 0);

	const uint8_t manufacturer[] = {'A', 0, 'B', 0, 'X', 0};
	const uint8_t product[] = {'C', 0, 'D', 0, 'X', 0};
	const uint8_t serial[] = {'S', 0, '1', 0, 'X', 0};
	const uint8_t factory_serial[] = "FACT1234";

	queue_flash_read(
		MCP2221_FLASH_DATA_USB_MANUFACTURER,
		6, manufacturer, sizeof(manufacturer));
	queue_flash_read(
		MCP2221_FLASH_DATA_USB_PRODUCT,
		6, product, sizeof(product));
	queue_flash_read(
		MCP2221_FLASH_DATA_USB_SERIALNUM,
		6, serial, sizeof(serial));
	queue_flash_read(
		MCP2221_FLASH_DATA_CHIP_SERIALNUM,
		8, factory_serial, sizeof(factory_serial) - 1);

	mcp2221_flash_info_t info;
	assert(mcp2221_flash_read_info(dev, &info) == MCP2221_ERR_OK);
	assert(strcmp(info.usb_manufacturer_str, "AB") == 0);
	assert(strcmp(info.usb_product_str, "CD") == 0);
	assert(strcmp(info.usb_serial_str, "S1") == 0);
	assert(strcmp(info.usb_factory_serial_str, "FACT1234") == 0);

	/* The public raw payload still starts at Read Flash response byte 4. */
	assert(info.usb_manufacturer[0] == 'A');
	assert(info.usb_manufacturer[1] == 0);
	assert(info.usb_manufacturer[4] == 'X');
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_flash_info_rejects_malformed_descriptor_metadata(void) {
	mcp2221_flash_info_t info;

	mcp2221_t *dev = open_test_device();
	queue_flash_read(MCP2221_FLASH_DATA_CHIP_SETTINGS, 0, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_GP_SETTINGS, 0, NULL, 0);

	const uint8_t odd_payload[] = {'A'};
	queue_flash_read(
		MCP2221_FLASH_DATA_USB_MANUFACTURER,
		3, odd_payload, sizeof(odd_payload));

	assert(mcp2221_flash_read_info(dev, &info) == MCP2221_ERR_PROTOCOL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);

	dev = open_test_device();
	queue_flash_read(MCP2221_FLASH_DATA_CHIP_SETTINGS, 0, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_GP_SETTINGS, 0, NULL, 0);

	uint8_t command[2] = {
		MCP2221_CMD_READ_FLASH_DATA,
		MCP2221_FLASH_DATA_USB_MANUFACTURER,
	};
	uint8_t response[MCP2221_PACKET_SIZE] = {0};
	response[MCP2221_RESPONSE_ECHO_BYTE] = MCP2221_CMD_READ_FLASH_DATA;
	response[MCP2221_RESPONSE_STATUS_BYTE] = MCP2221_RESPONSE_RESULT_OK;
	response[MCP2221_FLASH_RESPONSE_STRUCTURE_LENGTH_BYTE] = 4;
	response[MCP2221_TEST_FLASH_DESCRIPTOR_TYPE_BYTE] =
		(uint8_t)(LIBUSB_DT_STRING - 1u);
	response[MCP2221_FLASH_OFFSET_READ] = 'A';
	queue_success_response(command, sizeof(command), response);

	assert(mcp2221_flash_read_info(dev, &info) == MCP2221_ERR_PROTOCOL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);

	dev = open_test_device();
	queue_flash_read(MCP2221_FLASH_DATA_CHIP_SETTINGS, 0, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_GP_SETTINGS, 0, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_USB_MANUFACTURER, 2, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_USB_PRODUCT, 2, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_USB_SERIALNUM, 2, NULL, 0);
	queue_flash_read(
		MCP2221_FLASH_DATA_CHIP_SERIALNUM,
		(uint8_t)(MCP2221_FLASH_PAYLOAD_SIZE + 1u), NULL, 0);

	assert(mcp2221_flash_read_info(dev, &info) == MCP2221_ERR_PROTOCOL);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

static void test_flash_info_decodes_utf16_surrogates(void) {
	mcp2221_t *dev = open_test_device();

	queue_flash_read(MCP2221_FLASH_DATA_CHIP_SETTINGS, 0, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_GP_SETTINGS, 0, NULL, 0);

	/* "A" + U+1F600 GRINNING FACE + "B" in UTF-16LE. */
	const uint8_t manufacturer[] = {
		'A', 0,
		0x3D, 0xD8,
		0x00, 0xDE,
		'B', 0,
	};
	/* Isolated high surrogate followed by "X": malformed UTF-16. */
	const uint8_t product[] = {
		0x3D, 0xD8,
		'X', 0,
	};

	queue_flash_read(
		MCP2221_FLASH_DATA_USB_MANUFACTURER,
		10, manufacturer, sizeof(manufacturer));
	queue_flash_read(
		MCP2221_FLASH_DATA_USB_PRODUCT,
		6, product, sizeof(product));
	queue_flash_read(MCP2221_FLASH_DATA_USB_SERIALNUM, 2, NULL, 0);
	queue_flash_read(MCP2221_FLASH_DATA_CHIP_SERIALNUM, 0, NULL, 0);

	mcp2221_flash_info_t info;
	assert(mcp2221_flash_read_info(dev, &info) == MCP2221_ERR_OK);
	assert(strcmp(
		info.usb_manufacturer_str,
		"A\xF0\x9F\x98\x80" "B") == 0);
	assert(strcmp(
		info.usb_product_str,
		"\xEF\xBF\xBD" "X") == 0);
	assert(fake_libusb_all_expectations_met());
	mcp2221_close(dev);
}

int main(void) {
	test_open_discovers_hid_and_send_cmd_succeeds();
	test_send_cmd_maps_read_timeout();
	test_send_cmd_maps_write_timeout();
	test_send_cmd_preserves_non_timeout_read_error();
	test_send_cmd_rejects_short_read();
	test_i2c_transfers_propagate_preflight_status_timeout();
	test_i2c_transfers_propagate_preflight_release_line_errors();
	test_i2c_transfers_propagate_runtime_release_line_errors();
	test_i2c_speed_propagates_release_line_error();
	test_i2c_speed_rejects_invalid_response_status();
	test_i2c_status_rejects_invalid_line_levels();
	test_gpio_reads_reject_malformed_values();
	test_gpio_poll_helpers_share_timestamp_state();
	test_gpio_write_rejects_inconsistent_response();
	test_adc_read_raw_rejects_values_above_10_bits();
	test_ioc_read_rejects_malformed_state();
	test_sram_interrupt_keep_does_not_reuse_adc_bits();
	test_sram_cache_tracks_gpio_when_vrm_reclaim_fails();
	test_i2c_get_data_error_count_maps_i2c_error();
	test_i2c_rejects_chunk_larger_than_remaining_request();
	test_flash_save_config_uses_sram_payload_offsets();
	test_flash_info_uses_response_structure_lengths();
	test_flash_info_rejects_malformed_descriptor_metadata();
	test_flash_info_decodes_utf16_surrogates();
	return 0;
}
