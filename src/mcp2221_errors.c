#include "mcp2221_errors.h"

const char *mcp2221_error_code_to_string(mcp2221_error_code_t code) {
	switch (code) {
		case MCP2221_ERR_OK:
			return "OK";
		case MCP2221_ERR_USB:
			return "USBError";
		case MCP2221_ERR_NOT_ACK:
			return "NotAckError";
		case MCP2221_ERR_TIMEOUT:
			return "TimeoutError";
		case MCP2221_ERR_LOW_SCL:
			return "LowSCLError";
		case MCP2221_ERR_LOW_SDA:
			return "LowSDAError";
		case MCP2221_ERR_INVALID:
			return "InvalidAnswerError";
		case MCP2221_ERR_I2C:
			return "GenericI2CError";
		case MCP2221_ERR_FLASH_WRITE:
			return "FlashWriteError";
		case MCP2221_ERR_FLASH_PASSWD:
			return "FlashPasswordError";
		case MCP2221_ERR_GPIO_MODE:
			return "GPIOModeError";
		case MCP2221_ERR_I2C_SHORT_READ:
			return "I2CShortReadError";
		case MCP2221_ERR_FLASH_READ:
			return "FlashReadError";
		case MCP2221_ERR_GENERIC:
		default:
			return "GenericError";
	}
}
