/**
 * @file mcp2221_flash.h
 * @brief Low-level access to MCP2221 persistent flash sections.
 */

#ifndef MCP2221_FLASH_H
#define MCP2221_FLASH_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Read one MCP2221 flash section.
 *
 * The raw 60-byte payload returned by the selected section is copied to
 * @p out without further interpretation.
 *
 * Public section identifiers include:
 * - MCP2221_FLASH_DATA_CHIP_SETTINGS
 * - MCP2221_FLASH_DATA_GP_SETTINGS
 * - MCP2221_FLASH_DATA_USB_MANUFACTURER
 * - MCP2221_FLASH_DATA_USB_PRODUCT
 * - MCP2221_FLASH_DATA_USB_SERIALNUM
 * - MCP2221_FLASH_DATA_CHIP_SERIALNUM
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] section Flash section identifier. Must be one of the public
 *                    flash section identifiers.
 * @param[out] out Buffer receiving exactly 60 section-data bytes.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_FLASH_READ when the device
 *         rejects the flash-read command, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_read(mcp2221_t *dev, uint8_t section, uint8_t out[60]);

/**
 * @brief Write one MCP2221 flash section.
 *
 * The function sends the supplied 60-byte payload directly to the selected
 * writable persistent flash section.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] section Writable flash section identifier. Valid values are
 *                    MCP2221_FLASH_DATA_CHIP_SETTINGS through
 *                    MCP2221_FLASH_DATA_USB_SERIALNUM.
 *                    MCP2221_FLASH_DATA_CHIP_SERIALNUM is read-only.
 * @param[in] data Exactly 60 bytes of section data to write.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_FLASH_WRITE when the device
 *         rejects the flash-write command, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 *
 * @warning This is a low-level persistent write. The caller is responsible
 *          for supplying a valid payload for the selected section.
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_write(mcp2221_t *dev, uint8_t section, const uint8_t data[60]);

/**
 * @brief Send the eight-byte flash access password.
 *
 * This command supplies the password used by subsequent flash-write commands
 * on password-protected devices. The MCP2221 does not validate @p pwd when
 * this command is accepted; the password is checked only when a flash write
 * is attempted. MCP2221_ERR_OK therefore means that the password command was
 * accepted, not that @p pwd was correct.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] pwd Eight-byte flash access password.
 *
 * @return MCP2221_ERR_OK when the password command is accepted,
 *         MCP2221_ERR_FLASH_PASSWD when the device refuses the password
 *         command (for example after the failed-write password limit has
 *         been reached), MCP2221_ERR_INVALID for invalid arguments, or
 *         another mcp2221_error_code_t value on failure.
 *
 * @warning Three flash writes attempted with an incorrect supplied password
 *          exhaust the device's password-attempt limit. The MCP2221 then
 *          refuses further passwords until the device is reset. Do not retry
 *          password-protected flash writes blindly after a write fails.
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_send_password(mcp2221_t *dev, const uint8_t pwd[8]);

MCP2221_END_DECLS
#endif	// MCP2221_FLASH_H