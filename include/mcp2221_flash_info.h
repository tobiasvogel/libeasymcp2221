#ifndef MCP2221_FLASH_INFO_H
#define MCP2221_FLASH_INFO_H

#include <stdint.h>

#include "mcp2221.h"

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
} MCP2221_FlashInfo;

// Read all flash sections and parse USB strings (best-effort UTF16LE -> UTF8).
int mcp2221_flash_read_info(MCP2221 *dev, MCP2221_FlashInfo *info);

// Save current SRAM state (chip + GPIO) to flash, like Python save_config().
int mcp2221_flash_save_config(MCP2221 *dev);

#endif
