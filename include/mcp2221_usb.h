/**
 * @file mcp2221_usb.h
 * @brief USB enumeration attributes staged for persistent flash storage.
 */

#ifndef MCP2221_USB_H
#define MCP2221_USB_H

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Stage the USB Remote Wake-up capability setting.
 *
 * A value of 0 disables the capability; any nonzero value enables it.
 *
 * This function changes only the staged configuration stored in the device
 * handle. Call mcp2221_flash_save_config() to persist the setting in MCP2221
 * flash. The USB host sees the persisted value only after the device is
 * re-enumerated.
 *
 * Advertising Remote Wake-up capability does not by itself wake the host.
 * A suitable wake-up source, such as GP1 interrupt-on-change, must be
 * configured and the host operating system must permit the device to wake it.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] enable 0 to disable Remote Wake-up capability, nonzero to enable
 *                   it.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid device
 *         handle, or another mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_usb_get_remote_wakeup()
 * @see mcp2221_flash_save_config()
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_set_remote_wakeup(
	mcp2221_t *dev,
	int enable);

/**
 * @brief Return the effective USB Remote Wake-up setting.
 *
 * If mcp2221_usb_set_remote_wakeup() has staged a value that has not yet been
 * persisted with mcp2221_flash_save_config(), the staged value is returned.
 * Otherwise the value currently stored in MCP2221 flash is read and returned.
 *
 * This reports the effective library configuration. It does not report whether
 * the USB host currently has Remote Wake-up enabled for the device.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] enabled Receives 0 when disabled or 1 when enabled.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or an error returned while reading chip settings from
 *         flash.
 *
 * @see mcp2221_usb_set_remote_wakeup()
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_get_remote_wakeup(
	mcp2221_t *dev,
	int *enabled);

/**
 * @brief Stage whether the MCP2221 advertises itself as self-powered.
 *
 * A value of 0 advertises the device as bus-powered; any nonzero value
 * advertises it as self-powered.
 *
 * This setting changes only the USB enumeration attribute and does not change
 * the actual hardware power source. It should be enabled only when the
 * physical device is in fact self-powered.
 *
 * The setting remains staged in the device handle until
 * mcp2221_flash_save_config() persists it. The USB host observes the persisted
 * value only after re-enumeration.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] self_powered 0 for bus-powered, nonzero for self-powered.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid device
 *         handle, or another mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_usb_get_self_powered()
 * @see mcp2221_flash_save_config()
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_set_self_powered(
	mcp2221_t *dev,
	int self_powered);

/**
 * @brief Return the effective self-powered setting.
 *
 * A staged value takes precedence over the value currently stored in flash.
 * On success, the returned value is normalized to 0 or 1.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] self_powered Receives 0 for bus-powered or 1 for self-powered.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or an error returned while reading chip settings from
 *         flash.
 *
 * @see mcp2221_usb_set_self_powered()
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_get_self_powered(
	mcp2221_t *dev,
	int *self_powered);

/**
 * @brief Stage the USB bus current advertised by the MCP2221.
 *
 * The MCP2221 USBREQCRT field uses units of 2 mA. This API accepts the
 * requested current directly in milliamperes and performs the register
 * encoding internally. Valid values are even numbers from 0 through 500 mA;
 * for example, 100 mA is encoded as register value 50.
 *
 * This attribute only describes the device to the USB host. It does not
 * electrically limit, regulate, or switch current.
 *
 * The setting remains staged in the device handle until
 * mcp2221_flash_save_config() persists it. The USB host observes the persisted
 * value only after re-enumeration.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] ma Requested USB bus current in mA. Must be even and in the range
 *               0 through 500 inclusive.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid handle
 *         or unsupported current value, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @see mcp2221_usb_get_requested_current()
 * @see mcp2221_flash_save_config()
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_set_requested_current(
	mcp2221_t *dev,
	unsigned ma);

/**
 * @brief Return the effective requested USB bus current in milliamperes.
 *
 * A staged value takes precedence over the value currently stored in flash.
 * The returned value is decoded to milliamperes; it is not the raw USBREQCRT
 * register value.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] ma Receives the effective requested current in milliamperes.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or an error returned while reading chip settings from
 *         flash.
 *
 * @see mcp2221_usb_set_requested_current()
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_get_requested_current(
	mcp2221_t *dev,
	unsigned *ma);

MCP2221_END_DECLS

#endif // MCP2221_USB_H
