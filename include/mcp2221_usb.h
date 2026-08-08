#ifndef MCP2221_USB_H
#define MCP2221_USB_H

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * Configure whether the MCP2221 advertises USB Remote Wake-up capability.
 *
 * enable == 0 disables the capability; any nonzero value enables it.
 * The setting is staged until mcp2221_flash_save_config() is called and only
 * takes effect after the device is reset or reconnected and enumerated again.
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_set_remote_wakeup(
	mcp2221_t *dev,
	int enable);

/**
 * Return the effective Remote Wake-up setting.
 *
 * A staged value is returned when present; otherwise the value is read from
 * flash. *enabled is always normalized to 0 or 1.
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_get_remote_wakeup(
	mcp2221_t *dev,
	int *enabled);

/**
 * Configure whether the MCP2221 advertises itself as self-powered.
 *
 * self_powered == 0 selects bus-powered; any nonzero value selects
 * self-powered. This flag does not change the actual hardware power source.
 * Only enable it for hardware that is in fact self-powered.
 *
 * The setting is staged until mcp2221_flash_save_config() is called and only
 * takes effect after USB re-enumeration.
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_set_self_powered(
	mcp2221_t *dev,
	int self_powered);

/**
 * Return the effective self-powered setting, normalized to 0 or 1.
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_get_self_powered(
	mcp2221_t *dev,
	int *self_powered);

/**
 * Configure the USB bus current advertised by the MCP2221.
 *
 * The MCP2221 USBREQCRT field uses units of 2 mA. The public API therefore
 * accepts even values from 0 through 500 mA. This value only describes the
 * device to the USB host; it does not electrically limit or regulate current.
 *
 * The setting is staged until mcp2221_flash_save_config() is called and only
 * takes effect after USB re-enumeration.
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_set_requested_current(
	mcp2221_t *dev,
	unsigned ma);

/**
 * Return the effective requested USB bus current in mA.
 */
MCP2221_API mcp2221_error_code_t mcp2221_usb_get_requested_current(
	mcp2221_t *dev,
	unsigned *ma);

MCP2221_END_DECLS

#endif // MCP2221_USB_H
