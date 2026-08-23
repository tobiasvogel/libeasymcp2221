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
#include "mcp2221_internal_constants.h"

enum {
	MCP2221_TEST_FLASH_DESCRIPTOR_TYPE_BYTE = 3,
	MCP2221_TEST_I2C_GET_DATA_COUNT_BYTE = 3,
	MCP2221_TEST_GPIO_VALUE_LOW = 0x00u,
	MCP2221_TEST_GPIO_VALUE_HIGH = 0x01u,
	MCP2221_TEST_GPIO_VALUE_NOT_GPIO = 0xEEu,
	MCP2221_TEST_GPIO_VALUE_INVALID = 0x02u,
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
	test_send_cmd_rejects_short_read();
	test_gpio_reads_reject_malformed_values();
	test_adc_read_raw_rejects_values_above_10_bits();
	test_i2c_get_data_error_count_maps_i2c_error();
	test_i2c_rejects_chunk_larger_than_remaining_request();
	test_flash_info_uses_response_structure_lengths();
	test_flash_info_decodes_utf16_surrogates();
	return 0;
}
