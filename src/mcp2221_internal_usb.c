#include "mcp2221_internal_usb.h"

#include <string.h>

#include "mcp2221_internal_constants.h"

void mcp2221_internal_usb_state_clear(mcp2221_internal_usb_state_t *state) {
	if (!state)
		return;

	memset(state, 0, sizeof(*state));
}

mcp2221_error_code_t mcp2221_internal_usb_state_set_remote_wakeup(
	mcp2221_internal_usb_state_t *state,
	int enable) {
	if (!state)
		return MCP2221_ERR_INVALID;

	state->remote_wakeup = enable ? 1 : 0;
	state->remote_wakeup_valid = 1;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_usb_state_set_cdc_serial_enabled(
	mcp2221_internal_usb_state_t *state,
	int enable) {
	if (!state)
		return MCP2221_ERR_INVALID;

	state->cdc_serial_enabled = enable ? 1 : 0;
	state->cdc_serial_valid = 1;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_usb_state_set_self_powered(
	mcp2221_internal_usb_state_t *state,
	int self_powered) {
	if (!state)
		return MCP2221_ERR_INVALID;

	state->self_powered = self_powered ? 1 : 0;
	state->self_powered_valid = 1;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_usb_state_set_requested_current(
	mcp2221_internal_usb_state_t *state,
	unsigned ma) {
	if (!state)
		return MCP2221_ERR_INVALID;
	if (ma > MCP2221_USB_CURRENT_MAX_MA)
		return MCP2221_ERR_INVALID;
	if ((ma % MCP2221_USB_CURRENT_UNIT_MA) != 0)
		return MCP2221_ERR_INVALID;

	state->requested_current = (uint8_t)(ma / MCP2221_USB_CURRENT_UNIT_MA);
	state->requested_current_valid = 1;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_usb_state_apply_cdc_serial(
	const mcp2221_internal_usb_state_t *state,
	uint8_t current,
	uint8_t *out) {
	if (!state || !out)
		return MCP2221_ERR_INVALID;

	uint8_t value = current;
	if (state->cdc_serial_valid) {
		if (state->cdc_serial_enabled)
			value |= MCP2221_CDCSEC_CDCSNEN;
		else
			value &= (uint8_t)~MCP2221_CDCSEC_CDCSNEN;
	}

	*out = value;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_usb_state_apply_power_attr(
	const mcp2221_internal_usb_state_t *state,
	uint8_t current,
	uint8_t *out) {
	if (!state || !out)
		return MCP2221_ERR_INVALID;

	uint8_t value = current;

	if (state->remote_wakeup_valid) {
		if (state->remote_wakeup)
			value |= MCP2221_USB_PWR_REMOTE_WAKEUP;
		else
			value &= (uint8_t)~MCP2221_USB_PWR_REMOTE_WAKEUP;
	}

	if (state->self_powered_valid) {
		if (state->self_powered)
			value |= MCP2221_USB_PWR_SELF_POWERED;
		else
			value &= (uint8_t)~MCP2221_USB_PWR_SELF_POWERED;
	}

	*out = value;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_usb_state_apply_requested_current(
	const mcp2221_internal_usb_state_t *state,
	uint8_t current,
	uint8_t *out) {
	if (!state || !out)
		return MCP2221_ERR_INVALID;

	*out = state->requested_current_valid ? state->requested_current : current;
	return MCP2221_ERR_OK;
}
