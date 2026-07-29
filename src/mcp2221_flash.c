#include "mcp2221_flash.h"

#include <string.h>

#include "constants.h"
#include "exceptions.h"

int mcp2221_flash_read(mcp2221_t *dev, uint8_t section, uint8_t out[60]) {
	uint8_t buf[MCP2221_PACKET_SIZE] = {0};
	buf[0] = MCP2221_CMD_READ_FLASH_DATA;
	buf[1] = section;

	uint8_t resp[MCP2221_PACKET_SIZE];
	int err = mcp2221_send_cmd(dev, buf, MCP2221_PACKET_SIZE, resp);
	if (err)
		return err;

	// Returned data starts at offset MCP2221_FLASH_OFFSET_READ
	memcpy(out, &resp[MCP2221_FLASH_OFFSET_READ], 60);

	return MCP2221_ERR_OK;
}

int mcp2221_flash_write(mcp2221_t *dev, uint8_t section, const uint8_t data[60]) {
	uint8_t buf[MCP2221_PACKET_SIZE] = {0};
	buf[0] = MCP2221_CMD_WRITE_FLASH_DATA;
	buf[1] = section;

	// Data starts at MCP2221_FLASH_OFFSET_WRITE
	memcpy(&buf[MCP2221_FLASH_OFFSET_WRITE], data, 60);

	uint8_t resp[MCP2221_PACKET_SIZE];
	int err = mcp2221_send_cmd(dev, buf, MCP2221_PACKET_SIZE, resp);
	if (err)
		return err;

	// Flash write returns result code in resp[1] == 0 success
	if (resp[1] != 0x00)
		return MCP2221_ERR_FLASH_WRITE;

	return MCP2221_ERR_OK;
}

int mcp2221_flash_send_password(mcp2221_t *dev, const uint8_t pwd[8]) {
	uint8_t buf[MCP2221_PACKET_SIZE] = {0};
	buf[0] = MCP2221_CMD_SEND_FLASH_ACCESS_PASSWORD;

	memcpy(&buf[1], pwd, 8);

	uint8_t resp[MCP2221_PACKET_SIZE];
	int err = mcp2221_send_cmd(dev, buf, MCP2221_PACKET_SIZE, resp);
	if (err)
		return err;

	if (resp[1] != 0x00)
		return MCP2221_ERR_FLASH_PASSWD;

	return MCP2221_ERR_OK;
}
