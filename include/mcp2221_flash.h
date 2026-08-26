/**
 * @file mcp2221_flash.h
 * @brief Low-level access to MCP2221 persistent flash sections.
 */

#ifndef MCP2221_FLASH_H
#define MCP2221_FLASH_H

#include <stddef.h>
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
 * @brief Write one MCP2221 flash section using the legacy 60-byte payload.
 *
 * The 60 bytes are copied to Write Flash Data report bytes 2 through 61.
 * This is sufficient for chip settings, GP settings, and USB string
 * descriptors of up to 29 UTF-16 code units. Use mcp2221_flash_write_ex()
 * when the complete 62-byte write payload is required.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] section Writable flash section identifier. Valid values are
 *                    MCP2221_FLASH_DATA_CHIP_SETTINGS through
 *                    MCP2221_FLASH_DATA_USB_SERIALNUM.
 *                    MCP2221_FLASH_DATA_CHIP_SERIALNUM is read-only.
 * @param[in] data Exactly 60 Write Flash Data payload bytes, corresponding to
 *                 report bytes 2 through 61.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_FLASH_WRITE when the device
 *         rejects the flash-write command, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 *
 * @warning This is a low-level persistent write. The caller is responsible
 *          for supplying a valid payload for the selected section.
 *
 * @see mcp2221_flash_write_ex()
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_write(mcp2221_t *dev, uint8_t section, const uint8_t data[60]);

/**
 * @brief Write a length-delimited MCP2221 Write Flash Data payload.
 *
 * Bytes from @p data are copied starting at report byte 2, after the command
 * and flash-section bytes. Up to 62 bytes can therefore be supplied. This
 * permits a maximum-length MCP2221 USB string descriptor: its two descriptor
 * metadata bytes followed by 60 UTF-16LE data bytes.
 *
 * For USB manufacturer, product, and serial-number sections, @p data starts
 * with the USB descriptor length and descriptor type (0x03), followed by the
 * UTF-16LE string data. The caller remains responsible for constructing a
 * section-valid payload.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] section Writable flash section identifier. Valid values are
 *                    MCP2221_FLASH_DATA_CHIP_SETTINGS through
 *                    MCP2221_FLASH_DATA_USB_SERIALNUM.
 *                    MCP2221_FLASH_DATA_CHIP_SERIALNUM is read-only.
 * @param[in] data Write Flash Data payload copied to report byte 2 onward.
 * @param[in] data_len Number of bytes in @p data. Must be from 1 through 62.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_FLASH_WRITE when the device
 *         rejects the flash-write command, MCP2221_ERR_INVALID for an invalid
 *         section, pointer, or payload length, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @warning This is a low-level persistent write. The caller is responsible
 *          for supplying a valid payload for the selected section.
 */
MCP2221_API mcp2221_error_code_t mcp2221_flash_write_ex(
	mcp2221_t *dev, uint8_t section,
	const uint8_t *data, size_t data_len);

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