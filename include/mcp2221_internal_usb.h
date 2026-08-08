#ifndef MCP2221_INTERNAL_USB_H
#define MCP2221_INTERNAL_USB_H

/**
 * @file mcp2221_internal_usb.h
 * @brief Internal USB configuration helpers for libeasymcp2221.
 *
 * This header is private to the library and must not be installed or used by
 * applications. It stores USB settings that cannot be changed through the
 * normal MCP2221 SRAM configuration command and therefore have to be staged
 * until mcp2221_flash_save_config() persists them.
 */

#include <stdint.h>

#include "mcp2221.h"
#include "mcp2221_error_codes.h"

MCP2221_BEGIN_DECLS

typedef struct {
	int remote_wakeup_valid;
	int remote_wakeup;
	int self_powered_valid;
	int self_powered;
	int requested_current_valid;
	uint8_t requested_current;
} mcp2221_internal_usb_state_t;

void mcp2221_internal_usb_state_clear(mcp2221_internal_usb_state_t *state);

mcp2221_error_code_t mcp2221_internal_usb_state_set_remote_wakeup(
	mcp2221_internal_usb_state_t *state,
	int enable);

mcp2221_error_code_t mcp2221_internal_usb_state_set_self_powered(
	mcp2221_internal_usb_state_t *state,
	int self_powered);

mcp2221_error_code_t mcp2221_internal_usb_state_set_requested_current(
	mcp2221_internal_usb_state_t *state,
	unsigned ma);

/**
 * Apply staged USBPWRATTR changes to a byte read from flash.
 *
 * Only the SELFPWR and REMWKUP bits are touched. All other bits are preserved
 * exactly as they were read from flash.
 */
mcp2221_error_code_t mcp2221_internal_usb_state_apply_power_attr(
	const mcp2221_internal_usb_state_t *state,
	uint8_t current,
	uint8_t *out);

/**
 * Apply a staged USBREQCRT value to a byte read from flash.
 */
mcp2221_error_code_t mcp2221_internal_usb_state_apply_requested_current(
	const mcp2221_internal_usb_state_t *state,
	uint8_t current,
	uint8_t *out);

/**
 * Access the USB pending state stored in the opaque device handle.
 * These accessors are internal and do not transfer ownership.
 */
mcp2221_internal_usb_state_t *mcp2221_internal_usb_get_state(mcp2221_t *dev);
const mcp2221_internal_usb_state_t *mcp2221_internal_usb_get_state_const(const mcp2221_t *dev);

MCP2221_END_DECLS

#endif // MCP2221_INTERNAL_USB_H
