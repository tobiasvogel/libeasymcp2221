#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "mcp2221_internal_analog.h"

/*
 * Link stub required by mcp2221_internal_analog.c.
 * This test does not exercise device-level VDD storage.
 */
mcp2221_error_code_t mcp2221_internal_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts) {
	(void)dev;
	(void)volts;
	return MCP2221_ERR_INVALID;
}

static void test_zero(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			0.0,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 0);
}

static void test_single_step(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			1.0 / 32.0,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 1);
}

static void test_midpoint(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			0.5,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 16);
}

static void test_maximum(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			31.0 / 32.0,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 31);
}

static void test_truncates_between_steps(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			1.5 / 32.0,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 1);
}

static void test_rejects_invalid_values(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			-0.001,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			1.0,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			NAN,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_normalized_to_raw(
			0.5,
			NULL) == MCP2221_ERR_INVALID);
}

static void test_volts_zero(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			0.0,
			3.3,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 0);
}

static void test_volts_single_step(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			3.3 / 32.0,
			3.3,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 1);
}

static void test_volts_midpoint(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			1.65,
			3.3,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 16);
}

static void test_volts_maximum(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			4.096 * (31.0 / 32.0),
			4.096,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 31);
}

static void test_volts_truncates_between_steps(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			3.3 * (1.5 / 32.0),
			3.3,
			&raw) == MCP2221_ERR_OK);

	assert(raw == 1);
}

static void test_volts_rejects_invalid_values(void) {
	uint8_t raw;

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			-0.001,
			3.3,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			3.3,
			3.3,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			1.0,
			0.0,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			1.0,
			-3.3,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			NAN,
			3.3,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			1.0,
			NAN,
			&raw) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_dac_volts_to_raw(
			1.0,
			3.3,
			NULL) == MCP2221_ERR_INVALID);
}

int main(void) {
	test_zero();
	test_single_step();
	test_midpoint();
	test_maximum();
	test_truncates_between_steps();
	test_rejects_invalid_values();
    test_volts_zero();
    test_volts_single_step();
    test_volts_midpoint();
    test_volts_maximum();
    test_volts_truncates_between_steps();
    test_volts_rejects_invalid_values();

	return 0;
}