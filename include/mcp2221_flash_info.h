#ifndef MCP2221_FLASH_INFO_H
#define MCP2221_FLASH_INFO_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

typedef struct {
	uint8_t chip_settings[60];
	uint8_t gp_settings[60];
	uint8_t usb_manufacturer[60];
	uint8_t usb_product[60];
	uint8_t usb_serial[60];
	uint8_t usb_factory_serial[60];

	// Decoded UTF-8 strings (best-effort, null-terminated)
	char usb_manufacturer_str[128];
	char usb_product_str[128];
	char usb_serial_str[128];
	char usb_factory_serial_str[32];
} mcp2221_flash_info_t;

// Read all flash sections and parse USB strings (best-effort UTF16LE -> UTF8).
MCP2221_API mcp2221_error_code_t mcp2221_flash_read_info(mcp2221_t *dev, mcp2221_flash_info_t *info);

// Save current SRAM state (chip + GPIO) to flash, like Python save_config().
MCP2221_API mcp2221_error_code_t mcp2221_flash_save_config(mcp2221_t *dev);

MCP2221_END_DECLS
#endif // MCP2221_FLASH_INFO_H
