#ifndef MCP2221_FLASH_SETTINGS_H
#define MCP2221_FLASH_SETTINGS_H

#include <stdint.h>

#include "mcp2221.h"

typedef struct {
	uint8_t chip_settings[60];
	uint8_t gp_settings[60];
} MCP2221_FlashSettings;

int mcp2221_flash_get_settings(MCP2221 *dev, MCP2221_FlashSettings *st);

#endif	// MCP2221_FLASH_SETTINGS_H