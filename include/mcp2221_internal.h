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

/**
 * @internal
 * @brief Sends a command that is known to be safe to retry.
 *
 * Uses the device's configured command retry count. Callers must only use
 * this helper for operations whose side effects are safe when repeated.
 *
 * @param dev Device handle
 * @param buf Command bytes
 * @param len Number of command bytes
 * @param response Optional 64-byte response buffer
 * @return MCP2221_ERR_OK on success, another mcp2221_error_code_t value otherwise
 */
mcp2221_error_code_t mcp2221_internal_send_cmd_retry_safe(
	mcp2221_t *dev, const uint8_t *buf, size_t len, uint8_t *response);

/**
 * @internal
 * @brief Sends a command with retries limited to transport failures.
 *
 * Matches EasyMCP2221 semantics for commands such as GET_GPIO_VALUES:
 * retry USB/timeout failures, but do not repeat a successfully delivered
 * command solely because the MCP2221 returned a command-status failure.
 *
 * @param dev Device handle
 * @param buf Command bytes
 * @param len Number of command bytes
 * @param response Optional 64-byte response buffer
 * @return MCP2221_ERR_OK on success, another mcp2221_error_code_t value otherwise
 */
mcp2221_error_code_t mcp2221_internal_send_cmd_retry_transport(
	mcp2221_t *dev, const uint8_t *buf, size_t len, uint8_t *response);

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
 * @brief Read an MCP2221 flash section while preserving its structure length.
 *
 * The public mcp2221_flash_read() API intentionally returns only the 60-byte
 * section-data payload beginning at response byte 4. String parsers also need
 * response byte 2, which carries the section structure length.
 *
 * @param dev Device handle
 * @param section Flash section identifier
 * @param out Buffer receiving the 60-byte section-data payload
 * @param structure_length Optional output for response byte 2
 * @return MCP2221_ERR_OK on success, another mcp2221_error_code_t value otherwise
 */
mcp2221_error_code_t mcp2221_internal_flash_read(
	mcp2221_t *dev, uint8_t section, uint8_t out[60], uint8_t *structure_length);

/**
 * @internal
 * @brief Parse an MCP2221 USB string-descriptor payload into UTF-8.
 *
 * @p buf starts at response byte 4; @p structure_length is the value from
 * response byte 2 and includes the two-byte USB descriptor header.
 *
 * @param buf Flash section-data payload beginning at response byte 4
 * @param buf_len Number of bytes available in @p buf
 * @param structure_length Response byte 2 from Read Flash Data
 * @param out Output buffer
 * @param out_len Output buffer size in bytes
 */
void mcp2221_internal_parse_wchar_structure(
	const uint8_t *buf, size_t buf_len, uint8_t structure_length,
	char *out, size_t out_len);

/**
 * @internal
 * @brief Parse the raw MCP2221 factory serial-number payload.
 *
 * Unlike USB descriptor strings, the factory serial is an ordinary byte
 * string whose length is response byte 2 directly.
 */
void mcp2221_internal_parse_factory_serial(
	const uint8_t *buf, size_t buf_len, uint8_t structure_length,
	char *out, size_t out_len);

MCP2221_END_DECLS
#endif // MCP2221_INTERNAL_H
