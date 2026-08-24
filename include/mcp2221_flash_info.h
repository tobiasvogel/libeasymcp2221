/**
 * @file mcp2221_flash_info.h
 * @brief Aggregate flash-information and configuration-save helpers.
 */

#ifndef MCP2221_FLASH_INFO_H
#define MCP2221_FLASH_INFO_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Snapshot of the public MCP2221 flash-information sections.
 *
 * The raw arrays contain the complete 60-byte payloads returned by the device.
 * The string fields provide best-effort, null-terminated UTF-8 conversions of
 * the corresponding USB descriptor structures.
 */
typedef struct {
	/** @brief Raw chip-settings flash section. */
	uint8_t chip_settings[60];

	/** @brief Raw GP-settings flash section. */
	uint8_t gp_settings[60];

	/** @brief Raw USB manufacturer-string flash section. */
	uint8_t usb_manufacturer[60];

	/** @brief Raw USB product-string flash section. */
	uint8_t usb_product[60];

	/** @brief Raw USB serial-number flash section. */
	uint8_t usb_serial[60];

	/** @brief Raw factory/chip serial-number flash section. */
	uint8_t usb_factory_serial[60];

	/**
	 * @brief Decoded USB manufacturer string.
	 *
	 * Best-effort UTF-16LE-to-UTF-8 conversion, always null-terminated within
	 * this fixed-size buffer.
	 */
	char usb_manufacturer_str[128];

	/** @brief Decoded USB product string, best-effort UTF-8 and null-terminated. */
	char usb_product_str[128];

	/** @brief Decoded USB serial string, best-effort UTF-8 and null-terminated. */
	char usb_serial_str[128];

	/**
	 * @brief Decoded factory/chip serial string.
	 *
	 * Best-effort UTF-8 conversion, null-terminated within this fixed-size
	 * buffer.
	 */
	char usb_factory_serial_str[32];
} mcp2221_flash_info_t;

/**
 * @brief Read all public flash-information sections.
 *
 * The output structure is cleared first, then the chip settings, GP settings,
 * USB manufacturer, USB product, USB serial, and factory/chip serial sections
 * are read. USB descriptor metadata is validated before USB-style
 * wide-character structures are decoded to UTF-8 on a best-effort basis.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] info Receives raw flash sections and decoded strings.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, MCP2221_ERR_PROTOCOL for malformed string-descriptor
 *         metadata, or another error returned while reading flash.
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_read_info(mcp2221_t *dev, mcp2221_flash_info_t *info);

/**
 * @brief Save the current runtime configuration to persistent flash.
 *
 * The function updates the persistent chip-settings and GP-settings sections
 * from the current device SRAM state. GPIO values are taken from the library's
 * cached GPIO state when available so that changes made through the GPIO API
 * are retained.
 *
 * Enumeration-time USB power attributes and requested-current values remain
 * unchanged unless the corresponding USB setter has staged an explicit
 * update.
 *
 * @param[in] dev Open MCP2221 device handle.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid device
 *         handle or internal state, or another mcp2221_error_code_t value on
 *         failure.
 *
 * @warning This function performs persistent flash writes and is not atomic.
 *          The chip-settings section is written before the GP-settings
 *          section, so a later failure can leave the earlier section already
 *          persisted. Staged USB settings are cleared only after both writes
 *          succeed; they remain staged if the save fails.
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_save_config(mcp2221_t *dev);

MCP2221_END_DECLS
#endif // MCP2221_FLASH_INFO_H
