#include <assert.h>
#include <stdint.h>

#include "mcp2221_constants.h"
#include "mcp2221_internal_usb.h"

static void test_power_attr_preserves_unrelated_bits(void) {
	mcp2221_internal_usb_state_t state = {0};
	uint8_t out = 0;

	/* Deliberately keep unrelated/reserved bits set to verify preservation. */
	const uint8_t initial = 0x9Fu;

	assert(mcp2221_internal_usb_state_set_remote_wakeup(&state, 1) == MCP2221_ERR_OK);
	assert(mcp2221_internal_usb_state_apply_power_attr(&state, initial, &out) == MCP2221_ERR_OK);
	assert(out == (uint8_t)(initial | MCP2221_USB_PWR_REMOTE_WAKEUP));

	assert(mcp2221_internal_usb_state_set_remote_wakeup(&state, 0) == MCP2221_ERR_OK);
	assert(mcp2221_internal_usb_state_apply_power_attr(&state, initial, &out) == MCP2221_ERR_OK);
	assert(out == (uint8_t)(initial & (uint8_t)~MCP2221_USB_PWR_REMOTE_WAKEUP));
}

static void test_power_attr_combines_pending_flags(void) {
	mcp2221_internal_usb_state_t state = {0};
	uint8_t out = 0;

	assert(mcp2221_internal_usb_state_set_remote_wakeup(&state, 1) == MCP2221_ERR_OK);
	assert(mcp2221_internal_usb_state_set_self_powered(&state, 1) == MCP2221_ERR_OK);
	assert(mcp2221_internal_usb_state_apply_power_attr(&state, 0x80u, &out) == MCP2221_ERR_OK);
	assert(out == 0xE0u);

	mcp2221_internal_usb_state_clear(&state);
	assert(mcp2221_internal_usb_state_set_self_powered(&state, 1) == MCP2221_ERR_OK);
	assert(mcp2221_internal_usb_state_set_remote_wakeup(&state, 1) == MCP2221_ERR_OK);
	assert(mcp2221_internal_usb_state_apply_power_attr(&state, 0x80u, &out) == MCP2221_ERR_OK);
	assert(out == 0xE0u);
}

static void test_flags_accept_nonzero_and_normalize(void) {
	mcp2221_internal_usb_state_t state = {0};

	assert(mcp2221_internal_usb_state_set_remote_wakeup(&state, -7) == MCP2221_ERR_OK);
	assert(state.remote_wakeup_valid == 1);
	assert(state.remote_wakeup == 1);

	assert(mcp2221_internal_usb_state_set_self_powered(&state, 42) == MCP2221_ERR_OK);
	assert(state.self_powered_valid == 1);
	assert(state.self_powered == 1);

	assert(mcp2221_internal_usb_state_set_remote_wakeup(&state, 0) == MCP2221_ERR_OK);
	assert(state.remote_wakeup == 0);
}

static void test_requested_current(void) {
	mcp2221_internal_usb_state_t state = {0};
	uint8_t out = 0;

	assert(mcp2221_internal_usb_state_set_requested_current(&state, 0) == MCP2221_ERR_OK);
	assert(state.requested_current == 0);
	assert(mcp2221_internal_usb_state_apply_requested_current(&state, 50, &out) == MCP2221_ERR_OK);
	assert(out == 0);

	assert(mcp2221_internal_usb_state_set_requested_current(&state, 100) == MCP2221_ERR_OK);
	assert(state.requested_current == 50);
	assert(mcp2221_internal_usb_state_apply_requested_current(&state, 0, &out) == MCP2221_ERR_OK);
	assert(out == 50);

	assert(mcp2221_internal_usb_state_set_requested_current(&state, 500) == MCP2221_ERR_OK);
	assert(state.requested_current == 250);

	assert(mcp2221_internal_usb_state_set_requested_current(&state, 101) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_set_requested_current(&state, 502) == MCP2221_ERR_INVALID);
}

static void test_unset_state_preserves_flash_values(void) {
	mcp2221_internal_usb_state_t state = {0};
	uint8_t out = 0;

	assert(mcp2221_internal_usb_state_apply_power_attr(&state, 0xA5u, &out) == MCP2221_ERR_OK);
	assert(out == 0xA5u);

	assert(mcp2221_internal_usb_state_apply_requested_current(&state, 50u, &out) == MCP2221_ERR_OK);
	assert(out == 50u);
}

static void test_invalid_arguments(void) {
	mcp2221_internal_usb_state_t state = {0};
	uint8_t out = 0;

	assert(mcp2221_internal_usb_state_set_remote_wakeup(NULL, 1) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_set_self_powered(NULL, 1) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_set_requested_current(NULL, 100) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_apply_power_attr(NULL, 0, &out) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_apply_power_attr(&state, 0, NULL) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_apply_requested_current(NULL, 0, &out) == MCP2221_ERR_INVALID);
	assert(mcp2221_internal_usb_state_apply_requested_current(&state, 0, NULL) == MCP2221_ERR_INVALID);
}

int main(void) {
	test_power_attr_preserves_unrelated_bits();
	test_power_attr_combines_pending_flags();
	test_flags_accept_nonzero_and_normalize();
	test_requested_current();
	test_unset_state_preserves_flash_values();
	test_invalid_arguments();
	return 0;
}
