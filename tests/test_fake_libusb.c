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
	test_ioc_read_rejects_malformed_state();
	test_sram_interrupt_keep_does_not_reuse_adc_bits();
	test_i2c_get_data_error_count_maps_i2c_error();
	test_i2c_rejects_chunk_larger_than_remaining_request();
	test_flash_save_config_uses_sram_payload_offsets();
	test_flash_info_uses_response_structure_lengths();
	test_flash_info_decodes_utf16_surrogates();
	return 0;
}
