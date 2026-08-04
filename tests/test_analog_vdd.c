#include <assert.h>
#include <math.h>

#include "mcp2221_internal_analog.h"

/*
 * Link stub for mcp2221_internal_analog.c.
 *
 * The real implementation lives in mcp2221.c. This test exercises only the
 * standalone analog-state helpers, so the device-level accessor is not used.
 */
mcp2221_error_code_t mcp2221_internal_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts) {
	(void)dev;
	(void)volts;
	return MCP2221_ERR_INVALID;
}

static void assert_double_equal(double actual, double expected) {
	assert(fabs(actual - expected) < 1e-12);
}

static void test_initial_state_has_no_vdd(void) {
	mcp2221_internal_analog_state_t state = {0};
	double volts;

	assert(
		mcp2221_internal_analog_state_get_vdd(
			&state,
			&volts) == MCP2221_ERR_INVALID);
}

static void test_set_and_get_vdd(void) {
	mcp2221_internal_analog_state_t state = {0};
	double volts;

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			3.3) == MCP2221_ERR_OK);

	assert(
		mcp2221_internal_analog_state_get_vdd(
			&state,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(volts, 3.3);
}

static void test_vdd_boundary_values(void) {
	mcp2221_internal_analog_state_t state = {0};
	double volts;

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			MCP2221_MIN_VDD_VOLTS) == MCP2221_ERR_OK);

	assert(
		mcp2221_internal_analog_state_get_vdd(
			&state,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(volts, MCP2221_MIN_VDD_VOLTS);

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			MCP2221_MAX_VDD_VOLTS) == MCP2221_ERR_OK);

	assert(
		mcp2221_internal_analog_state_get_vdd(
			&state,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(volts, MCP2221_MAX_VDD_VOLTS);
}

static void test_rejects_invalid_voltages(void) {
	mcp2221_internal_analog_state_t state = {0};

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			MCP2221_MIN_VDD_VOLTS - 0.001) ==
		MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			MCP2221_MAX_VDD_VOLTS + 0.001) ==
		MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			NAN) == MCP2221_ERR_INVALID);
}

static void test_invalid_arguments(void) {
	mcp2221_internal_analog_state_t state = {0};
	double volts;

	assert(
		mcp2221_internal_analog_state_set_vdd(
			NULL,
			3.3) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_state_get_vdd(
			NULL,
			&volts) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_state_get_vdd(
			&state,
			NULL) == MCP2221_ERR_INVALID);
}

static void test_invalid_update_preserves_previous_vdd(void) {
	mcp2221_internal_analog_state_t state = {0};
	double volts;

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			3.3) == MCP2221_ERR_OK);

	assert(
		mcp2221_internal_analog_state_set_vdd(
			&state,
			6.0) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_state_get_vdd(
			&state,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(volts, 3.3);
}

int main(void) {
	test_initial_state_has_no_vdd();
	test_set_and_get_vdd();
	test_vdd_boundary_values();
	test_rejects_invalid_voltages();
	test_invalid_arguments();
	test_invalid_update_preserves_previous_vdd();

	return 0;
}