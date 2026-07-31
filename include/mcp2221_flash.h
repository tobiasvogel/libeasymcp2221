#ifndef MCP2221_FLASH_H
#define MCP2221_FLASH_H

#include <stdint.h>

#include "mcp2221.h"

/* Read a flash section.
 * Output buffer must be 60 bytes.
 *
 * Returns MCP2221_ERR_OK on success or another error code on failure.
 */
mcp2221_error_code_t mcp2221_flash_read(mcp2221_t *dev, uint8_t section, uint8_t out[60]);

/* Write a flash section.
 * Input buffer must be 60 bytes.
 *
 * Returns MCP2221_ERR_OK on success or another error code on failure.
 */
mcp2221_error_code_t mcp2221_flash_write(mcp2221_t *dev, uint8_t section, const uint8_t data[60]);

/* Send 8-byte password for unlocking protected flash */
mcp2221_error_code_t mcp2221_flash_send_password(mcp2221_t *dev, const uint8_t pwd[8]);

#endif	// MCP2221_FLASH_H