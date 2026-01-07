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
#include "error_codes.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration (MCP2221 is opaque in public API)
typedef struct MCP2221 MCP2221;

/**
 * @internal
 * @brief Ensures GPIO status cache is loaded from device SRAM
 * 
 * Reads the current GPIO configuration from the device if not already cached.
 * This corresponds to Python EasyMCP2221's _ensure_gpio_status() method.
 * 
 * @param dev Device handle
 * @return MCP_ERR_OK on success, error code otherwise
 */
mcp_err_t mcp2221_internal_ensure_gpio_status(MCP2221 *dev);

/**
 * @internal
 * @brief Gets cached GPIO status for all 4 pins
 * 
 * Returns the cached GPIO configuration bytes (GP0..GP3).
 * Cache must be valid (call ensure_gpio_status first).
 * 
 * @param dev Device handle
 * @param out_gp Output buffer (4 bytes) for GP0..GP3 config
 * @return MCP_ERR_OK on success, MCP_ERR_INVALID if cache not valid
 */
mcp_err_t mcp2221_internal_gpio_status_get(MCP2221 *dev, uint8_t out_gp[4]);

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
void mcp2221_internal_gpio_status_set(MCP2221 *dev, const uint8_t gp[4]);

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
void mcp2221_internal_gpio_status_update_out(MCP2221 *dev, int pin, int out_value);

#ifdef __cplusplus
}
#endif

#endif // MCP2221_INTERNAL_H
