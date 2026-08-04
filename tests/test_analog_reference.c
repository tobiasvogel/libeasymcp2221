#include <assert.h>
#include <math.h>
#include <stddef.h>

#include "mcp2221_constants.h"
#include "mcp2221_internal_analog.h"

/*
 * Test stub required by mcp2221_internal_analog_get_reference_voltage().
 *
 * The actual implementation lives in mcp2221.c and accesses the opaque
 * device state. This test exercises the analog reference module in isolation.
 */
mcp2221_error_code_t mcp2221_internal_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts) {
	if (!dev || !volts)
		return MCP2221_ERR_INVALID;

	*volts = 3.3;
	return MCP2221_ERR_OK;
}

static void assert_double_equal(double actual, double expected) {
	assert(fabs(actual - expected) < 1e-12);
}

static void test_parse_valid_references(void) {
	mcp2221_analog_voltage_reference_t reference;

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"OFF",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_OFF);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"VDD",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_VDD);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"1.024V",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_1_024V);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"2.048V",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_2_048V);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"4.096V",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_4_096V);
}

static void test_parse_is_case_insensitive(void) {
	mcp2221_analog_voltage_reference_t reference;

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"vdd",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_VDD);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"1.024v",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_1_024V);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"Off",
			&reference) == MCP2221_ERR_OK);
	assert(reference == MCP2221_ANALOG_VOLTAGE_REF_OFF);
}

static void test_parse_rejects_invalid_arguments(void) {
	mcp2221_analog_voltage_reference_t reference;

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			NULL,
			&reference) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"VDD",
			NULL) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_parse_voltage_reference(
			"invalid",
			&reference) == MCP2221_ERR_INVALID);
}

static void test_adc_reference_bits(void) {
	int bits;

	assert(
		mcp2221_internal_analog_adc_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_OFF,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_OFF));

	assert(
		mcp2221_internal_analog_adc_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_VDD,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_ADC_REF_VDD | MCP2221_ADC_VRM_OFF));

	assert(
		mcp2221_internal_analog_adc_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_1_024V,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_1024));

	assert(
		mcp2221_internal_analog_adc_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_2_048V,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_2048));

	assert(
		mcp2221_internal_analog_adc_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_4_096V,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_4096));

	assert(
		mcp2221_internal_analog_adc_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_VDD,
			NULL) == MCP2221_ERR_INVALID);
}

static void test_dac_reference_bits(void) {
	int bits;

	assert(
		mcp2221_internal_analog_dac_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_OFF,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_OFF));

	assert(
		mcp2221_internal_analog_dac_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_VDD,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_DAC_REF_VDD | MCP2221_DAC_VRM_OFF));

	assert(
		mcp2221_internal_analog_dac_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_1_024V,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_1024));

	assert(
		mcp2221_internal_analog_dac_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_2_048V,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_2048));

	assert(
		mcp2221_internal_analog_dac_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_4_096V,
			&bits) == MCP2221_ERR_OK);
	assert(bits == (MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_4096));

	assert(
		mcp2221_internal_analog_dac_reference_to_bits(
			MCP2221_ANALOG_VOLTAGE_REF_VDD,
			NULL) == MCP2221_ERR_INVALID);
}

static void test_reference_voltages(void) {
	double volts;

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			NULL,
			MCP2221_ANALOG_VOLTAGE_REF_1_024V,
			&volts) == MCP2221_ERR_OK);
	assert_double_equal(volts, 1.024);

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			NULL,
			MCP2221_ANALOG_VOLTAGE_REF_2_048V,
			&volts) == MCP2221_ERR_OK);
	assert_double_equal(volts, 2.048);

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			NULL,
			MCP2221_ANALOG_VOLTAGE_REF_4_096V,
			&volts) == MCP2221_ERR_OK);
	assert_double_equal(volts, 4.096);

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			NULL,
			MCP2221_ANALOG_VOLTAGE_REF_OFF,
			&volts) == MCP2221_ERR_INVALID);

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			NULL,
			MCP2221_ANALOG_VOLTAGE_REF_1_024V,
			NULL) == MCP2221_ERR_INVALID);
}

static void test_vdd_reference_uses_device_state_accessor(void) {
	/*
	 * The stub only checks for a non-NULL handle; it never dereferences it.
	 */
	const mcp2221_t *fake_dev = (const mcp2221_t *)1;
	double volts;

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			fake_dev,
			MCP2221_ANALOG_VOLTAGE_REF_VDD,
			&volts) == MCP2221_ERR_OK);
	assert_double_equal(volts, 3.3);

	assert(
		mcp2221_internal_analog_get_reference_voltage(
			NULL,
			MCP2221_ANALOG_VOLTAGE_REF_VDD,
			&volts) == MCP2221_ERR_INVALID);
}

int main(void) {
	test_parse_valid_references();
	test_parse_is_case_insensitive();
	test_parse_rejects_invalid_arguments();
	test_adc_reference_bits();
	test_dac_reference_bits();
	test_reference_voltages();
	test_vdd_reference_uses_device_state_accessor();

	return 0;
}