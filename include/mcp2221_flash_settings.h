/**
 * @file mcp2221_flash_settings.h
 * @brief Raw persistent chip and GP flash settings.
 */

#ifndef MCP2221_FLASH_SETTINGS_H
#define MCP2221_FLASH_SETTINGS_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Raw persistent chip-settings and GP-settings snapshot.
 *
 * The buffers contain the complete 60-byte flash-section payloads without
 * decoding or modification.
 */
typedef struct {
	/** @brief Raw MCP2221_FLASH_DATA_CHIP_SETTINGS payload. */
	uint8_t chip_settings[60];

	/** @brief Raw MCP2221_FLASH_DATA_GP_SETTINGS payload. */
	uint8_t gp_settings[60];
} mcp2221_flash_settings_t;

/**
 * @brief Read the persistent chip-settings and GP-settings flash sections.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] st Receives both raw 60-byte flash sections.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or an error returned by mcp2221_flash_read().
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_get_settings(mcp2221_t *dev, mcp2221_flash_settings_t *st);

MCP2221_END_DECLS
#endif	// MCP2221_FLASH_SETTINGS_H