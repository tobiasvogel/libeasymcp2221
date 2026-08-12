#include "mcp2221_usb.h"

#include "mcp2221_internal_constants.h"
#include "mcp2221_flash.h"
#include "mcp2221_internal_usb.h"

static mcp2221_error_code_t read_chip_settings(mcp2221_t *dev, uint8_t chip[60]) {
	if (!dev || !chip)
		return MCP2221_ERR_INVALID;

	mcp2221_error_code_t err = mcp2221_flash_read(dev, MCP2221_FLASH_DATA_CHIP_SETTINGS, chip);
	if (err != MCP2221_ERR_OK)
		return MCP2221_ERR_FLASH_READ;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_usb_set_remote_wakeup(mcp2221_t *dev, int enable) {
	mcp2221_internal_usb_state_t *state = mcp2221_internal_usb_get_state(dev);
	if (!state)
		return MCP2221_ERR_INVALID;

	return mcp2221_internal_usb_state_set_remote_wakeup(state, enable);
}

/*
 * Return the effective Remote Wake-up setting.
 *
 * If a value has been staged with mcp2221_usb_set_remote_wakeup()
 * but not yet persisted with mcp2221_flash_save_config(), the staged
 * value is returned. Otherwise the current value is read from flash.
 *
 * On success, *enabled is always 0 or 1.
 */
mcp2221_error_code_t mcp2221_usb_get_remote_wakeup(mcp2221_t *dev, int *enabled) {
	if (!dev || !enabled)
		return MCP2221_ERR_INVALID;

	const mcp2221_internal_usb_state_t *state = mcp2221_internal_usb_get_state_const(dev);
	if (!state)
		return MCP2221_ERR_INVALID;

	if (state->remote_wakeup_valid) {
		*enabled = state->remote_wakeup ? 1 : 0;
		return MCP2221_ERR_OK;
	}

	uint8_t chip[60];
	mcp2221_error_code_t err = read_chip_settings(dev, chip);
	if (err != MCP2221_ERR_OK)
		return err;

	*enabled = (chip[MCP2221_FLASH_CHIP_SETTINGS_USBPWR] & MCP2221_USB_PWR_REMOTE_WAKEUP) ? 1 : 0;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_usb_set_self_powered(mcp2221_t *dev, int self_powered) {
	mcp2221_internal_usb_state_t *state = mcp2221_internal_usb_get_state(dev);
	if (!state)
		return MCP2221_ERR_INVALID;

	return mcp2221_internal_usb_state_set_self_powered(state, self_powered);
}

mcp2221_error_code_t mcp2221_usb_get_self_powered(mcp2221_t *dev, int *self_powered) {
	if (!dev || !self_powered)
		return MCP2221_ERR_INVALID;

	const mcp2221_internal_usb_state_t *state = mcp2221_internal_usb_get_state_const(dev);
	if (!state)
		return MCP2221_ERR_INVALID;

	if (state->self_powered_valid) {
		*self_powered = state->self_powered ? 1 : 0;
		return MCP2221_ERR_OK;
	}

	uint8_t chip[60];
	mcp2221_error_code_t err = read_chip_settings(dev, chip);
	if (err != MCP2221_ERR_OK)
		return err;

	*self_powered = (chip[MCP2221_FLASH_CHIP_SETTINGS_USBPWR] & MCP2221_USB_PWR_SELF_POWERED) ? 1 : 0;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_usb_set_requested_current(mcp2221_t *dev, unsigned ma) {
	mcp2221_internal_usb_state_t *state = mcp2221_internal_usb_get_state(dev);
	if (!state)
		return MCP2221_ERR_INVALID;

	return mcp2221_internal_usb_state_set_requested_current(state, ma);
}

mcp2221_error_code_t mcp2221_usb_get_requested_current(mcp2221_t *dev, unsigned *ma) {
	if (!dev || !ma)
		return MCP2221_ERR_INVALID;

	const mcp2221_internal_usb_state_t *state = mcp2221_internal_usb_get_state_const(dev);
	if (!state)
		return MCP2221_ERR_INVALID;

	uint8_t encoded;
	if (state->requested_current_valid) {
		encoded = state->requested_current;
	} else {
		uint8_t chip[60];
		mcp2221_error_code_t err = read_chip_settings(dev, chip);
		if (err != MCP2221_ERR_OK)
			return err;
		encoded = chip[MCP2221_FLASH_CHIP_SETTINGS_USBMA];
	}

	*ma = (unsigned)encoded * MCP2221_USB_CURRENT_UNIT_MA;
	return MCP2221_ERR_OK;
}
