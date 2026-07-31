#ifndef MCP2221_FLASH_SETTINGS_H
#define MCP2221_FLASH_SETTINGS_H

#include <stdint.h>

#include "mcp2221.h"

typedef struct {
	uint8_t chip_settings[60];
	uint8_t gp_settings[60];
} mcp2221_flash_settings_t;

MCP2221_API int mcp2221_flash_get_settings(mcp2221_t *dev, mcp2221_flash_settings_t *st);

#endif	// MCP2221_FLASH_SETTINGS_H