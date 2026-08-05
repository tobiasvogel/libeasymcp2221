#include <assert.h>
#include <math.h>
#include <stddef.h>

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

static void assert_double_equal(double actual, double expected) {
	assert(fabs(actual - expected) < 1e-12);
}

static void test_zero(void) {
	double normalized;

	assert(
		mcp2221_internal_analog_adc_raw_to_normalized(
			0,
			&normalized) == MCP2221_ERR_OK);

	assert_double_equal(normalized, 0.0);
}

static void test_midpoint(void) {
	double normalized;

	assert(
		mcp2221_internal_analog_adc_raw_to_normalized(
			512,
			&normalized) == MCP2221_ERR_OK);

	assert_double_equal(normalized, 0.5);
}

static void test_maximum(void) {
	double normalized;

	assert(
		mcp2221_internal_analog_adc_raw_to_normalized(
			1023,
			&normalized) == MCP2221_ERR_OK);

	assert_double_equal(normalized, 1023.0 / 1024.0);
}

static void test_rejects_out_of_range_value(void) {
	double normalized;

	assert(
		mcp2221_internal_analog_adc_raw_to_normalized(
			1024,
			&normalized) == MCP2221_ERR_INVALID);
}

static void test_rejects_null_output(void) {
	assert(
		mcp2221_internal_analog_adc_raw_to_normalized(
			512,
			NULL) == MCP2221_ERR_INVALID);
}

static void test_raw_to_volts_zero(void) {
	double volts;

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			0,
			3.3,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(volts, 0.0);
}

static void test_raw_to_volts_midpoint(void) {
	double volts;

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			512,
			3.3,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(volts, 1.65);
}

static void test_raw_to_volts_maximum(void) {
	double volts;

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			1023,
			4.096,
			&volts) == MCP2221_ERR_OK);

	assert_double_equal(
		volts,
		(1023.0 / 1024.0) * 4.096);
}

static void test_raw_to_volts_rejects_invalid_input(void) {
	double volts;

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			1024,
			3.3,
			&volts) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			512,
			0.0,
			&volts) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			512,
			-1.0,
			&volts) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			512,
			NAN,
			&volts) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_adc_raw_to_volts(
			512,
			3.3,
			NULL) == MCP2221_ERR_INVALID);
}

int main(void) {
	test_zero();
	test_midpoint();
	test_maximum();
	test_rejects_out_of_range_value();
	test_rejects_null_output();
	test_raw_to_volts_zero();
	test_raw_to_volts_midpoint();
	test_raw_to_volts_maximum();
	test_raw_to_volts_rejects_invalid_input();

	return 0;
}