#include "mcp2221_flash.h"

#include <string.h>

#include "mcp2221_internal_constants.h"
#include "mcp2221_internal.h"
#include "mcp2221_errors.h"

static int is_valid_flash_read_section(uint8_t section) {
	return section <= MCP2221_FLASH_DATA_CHIP_SERIALNUM;
}

static int is_valid_flash_write_section(uint8_t section) {
	return section <= MCP2221_FLASH_DATA_USB_SERIALNUM;
}

static int is_usb_string_descriptor_section(uint8_t section) {
	return section == MCP2221_FLASH_DATA_USB_MANUFACTURER ||
	       section == MCP2221_FLASH_DATA_USB_PRODUCT ||
	       section == MCP2221_FLASH_DATA_USB_SERIALNUM;
}

static mcp2221_error_code_t validate_structure_metadata(
	uint8_t section, const uint8_t resp[MCP2221_PACKET_SIZE]) {
	uint8_t structure_length =
		resp[MCP2221_FLASH_RESPONSE_STRUCTURE_LENGTH_BYTE];

	if (is_usb_string_descriptor_section(section)) {
		if (resp[MCP2221_FLASH_RESPONSE_DESCRIPTOR_TYPE_BYTE] !=
		    MCP2221_USB_STRING_DESCRIPTOR_TYPE)
			return MCP2221_ERR_PROTOCOL;
		if (structure_length < MCP2221_USB_STRING_DESCRIPTOR_HEADER_SIZE)
			return MCP2221_ERR_PROTOCOL;

		size_t payload_length =
			(size_t)structure_length -
			MCP2221_USB_STRING_DESCRIPTOR_HEADER_SIZE;
		if (payload_length > MCP2221_FLASH_PAYLOAD_SIZE ||
		    (payload_length % 2u) != 0)
			return MCP2221_ERR_PROTOCOL;
	} else if (section == MCP2221_FLASH_DATA_CHIP_SERIALNUM &&
	           structure_length > MCP2221_FLASH_PAYLOAD_SIZE) {
		return MCP2221_ERR_PROTOCOL;
	}

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_flash_read(
	mcp2221_t *dev, uint8_t section, uint8_t out[60], uint8_t *structure_length) {
	if (!dev || !out || !is_valid_flash_read_section(section))
		return MCP2221_ERR_INVALID;
	if (structure_length)
		*structure_length = 0;

	uint8_t buf[MCP2221_PACKET_SIZE] = {0};
	buf[0] = MCP2221_CMD_READ_FLASH_DATA;
	buf[1] = section;

	uint8_t resp[MCP2221_PACKET_SIZE];
	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_safe(dev, buf, MCP2221_PACKET_SIZE, resp);
	if (err == MCP2221_ERR_COMMAND_FAILED)
		return MCP2221_ERR_FLASH_READ;
	if (err)
		return err;

	if (structure_length) {
		err = validate_structure_metadata(section, resp);
		if (err != MCP2221_ERR_OK)
			return err;
		*structure_length =
			resp[MCP2221_FLASH_RESPONSE_STRUCTURE_LENGTH_BYTE];
	}

	// Returned data starts at offset MCP2221_FLASH_OFFSET_READ
	memcpy(out, &resp[MCP2221_FLASH_OFFSET_READ], MCP2221_FLASH_PAYLOAD_SIZE);

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_flash_read(mcp2221_t *dev, uint8_t section, uint8_t out[60]) {
	return mcp2221_internal_flash_read(dev, section, out, NULL);
}

mcp2221_error_code_t mcp2221_flash_write(mcp2221_t *dev, uint8_t section, const uint8_t data[60]) {
	if (!dev || !data || !is_valid_flash_write_section(section))
		return MCP2221_ERR_INVALID;

	uint8_t buf[MCP2221_PACKET_SIZE] = {0};
	buf[0] = MCP2221_CMD_WRITE_FLASH_DATA;
	buf[1] = section;

	// Data starts at MCP2221_FLASH_OFFSET_WRITE
	memcpy(&buf[MCP2221_FLASH_OFFSET_WRITE], data, 60);

	uint8_t resp[MCP2221_PACKET_SIZE];
	mcp2221_error_code_t err = mcp2221_send_cmd(dev, buf, MCP2221_PACKET_SIZE, resp);
	if (err == MCP2221_ERR_COMMAND_FAILED)
		return MCP2221_ERR_FLASH_WRITE;
	if (err)
		return err;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_flash_send_password(mcp2221_t *dev, const uint8_t pwd[8]) {
	if (!dev || !pwd)
		return MCP2221_ERR_INVALID;

	uint8_t buf[MCP2221_PACKET_SIZE] = {0};
	buf[0] = MCP2221_CMD_SEND_FLASH_ACCESS_PASSWORD;

	memcpy(&buf[MCP2221_FLASH_PASSWORD_OFFSET], pwd, 8);

	uint8_t resp[MCP2221_PACKET_SIZE];
	mcp2221_error_code_t err = mcp2221_send_cmd(dev, buf, MCP2221_PACKET_SIZE, resp);
	if (err == MCP2221_ERR_COMMAND_FAILED)
		return MCP2221_ERR_FLASH_PASSWD;
	if (err)
		return err;

	return MCP2221_ERR_OK;
}
