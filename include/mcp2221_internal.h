#ifndef MCP2221_INTERNAL_H
#define MCP2221_INTERNAL_H

/**
 * @file mcp2221_internal.h
 * @brief Internal API for libeasymcp2221 - NOT for external use
 *
 * This header contains functions that are shared between multiple
 * implementation files but are not part of the public API.
 * External applications should NOT include this header.
 */

#include "mcp2221.h"
#include "mcp2221_error_codes.h"
#include <stdint.h>

MCP2221_BEGIN_DECLS

typedef enum {
	MCP2221_ANALOG_VOLTAGE_REF_OFF,
	MCP2221_ANALOG_VOLTAGE_REF_VDD,
	MCP2221_ANALOG_VOLTAGE_REF_1_024V,
	MCP2221_ANALOG_VOLTAGE_REF_2_048V,
	MCP2221_ANALOG_VOLTAGE_REF_4_096V
} mcp2221_analog_voltage_reference_t;

/**
 * @internal
 * @brief Ensures GPIO status cache is loaded from device SRAM
 *
 * Reads the current GPIO configuration from the device if not already cached.
 * This corresponds to Python EasyMCP2221's _ensure_gpio_status() method.
 *
 * @param dev Device handle
 * @return MCP2221_ERR_OK on success, another mcp2221_error_code_t value otherwise
 */
mcp2221_error_code_t mcp2221_internal_ensure_gpio_status(mcp2221_t *dev);

/**
 * @internal
 * @brief Gets cached GPIO status for all 4 pins
 *
 * Returns the cached GPIO configuration bytes (GP0..GP3).
 * Cache must be valid (call ensure_gpio_status first).
 *
 * @param dev Device handle
 * @param out_gp Output buffer (4 bytes) for GP0..GP3 config
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID if the cache is not valid
 */
mcp2221_error_code_t mcp2221_internal_gpio_status_get(mcp2221_t *dev, uint8_t out_gp[4]);

/**
 * @internal
 * @brief Sets cached GPIO status for all 4 pins
 *
 * Updates the internal cache with new GPIO configuration bytes.
 * Does NOT write to device - only updates cache.
 *
 * @param dev Device handle
 * @param gp Input buffer (4 bytes) with GP0..GP3 config
 */
void mcp2221_internal_gpio_status_set(mcp2221_t *dev, const uint8_t gp[4]);

/**
 * @internal
 * @brief Updates single GPIO output value in cache
 *
 * Sets or clears the output value bit for a specific GPIO pin
 * in the cached configuration. Does NOT write to device.
 *
 * @param dev Device handle
 * @param pin Pin number (0..3)
 * @param out_value Output value (0=low, non-zero=high)
 */
void mcp2221_internal_gpio_status_update_out(mcp2221_t *dev, int pin, int out_value);

/**
 * @internal
 * @brief Converts UTF-16LE bytes to UTF-8.
 *
 * Best-effort BMP-only conversion used for USB string descriptors stored in MCP2221 flash.
 * The output buffer is always NUL-terminated when out_len is greater than zero.
 *
 * @param in UTF-16LE input bytes
 * @param in_len Number of input bytes
 * @param out Output buffer
 * @param out_len Output buffer size in bytes
 */
void mcp2221_internal_utf16le_to_utf8(const uint8_t *in, size_t in_len, char *out, size_t out_len);

/**
 * @internal
 * @brief Parses MCP2221 flash wchar/string structures into UTF-8.
 *
 * MCP2221 flash string blocks store the byte length at buf[2] and UTF-16LE data starting at buf[4].
 * The parser caps the declared string payload at the 56 bytes available in the 60-byte flash payload.
 *
 * @param buf 60-byte MCP2221 flash string block
 * @param out Output buffer
 * @param out_len Output buffer size in bytes
 */
void mcp2221_internal_parse_wchar_structure(const uint8_t *buf, char *out, size_t out_len);

MCP2221_END_DECLS
#endif // MCP2221_INTERNAL_H
