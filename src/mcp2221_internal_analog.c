#include "mcp2221_internal_analog.h"

#include <strings.h>

#include "mcp2221_constants.h"

mcp2221_error_code_t mcp2221_internal_analog_parse_voltage_reference(
	const char *ref_str,
	mcp2221_analog_voltage_reference_t *reference) {
	if (!ref_str || !reference)
		return MCP2221_ERR_INVALID;

	if (strcasecmp(ref_str, "OFF") == 0) {
		*reference = MCP2221_ANALOG_VOLTAGE_REF_OFF;
	} else if (strcasecmp(ref_str, "VDD") == 0) {
		*reference = MCP2221_ANALOG_VOLTAGE_REF_VDD;
	} else if (strcasecmp(ref_str, "1.024V") == 0) {
		*reference = MCP2221_ANALOG_VOLTAGE_REF_1_024V;
	} else if (strcasecmp(ref_str, "2.048V") == 0) {
		*reference = MCP2221_ANALOG_VOLTAGE_REF_2_048V;
	} else if (strcasecmp(ref_str, "4.096V") == 0) {
		*reference = MCP2221_ANALOG_VOLTAGE_REF_4_096V;
	} else {
		return MCP2221_ERR_INVALID;
	}

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_adc_reference_to_bits(
	mcp2221_analog_voltage_reference_t reference,
	int *bits) {
	if (!bits)
		return MCP2221_ERR_INVALID;

	switch (reference) {
	case MCP2221_ANALOG_VOLTAGE_REF_OFF:
		*bits = MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_OFF;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_VDD:
		*bits = MCP2221_ADC_REF_VDD | MCP2221_ADC_VRM_OFF;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_1_024V:
		*bits = MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_1024;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_2_048V:
		*bits = MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_2048;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_4_096V:
		*bits = MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_4096;
		break;

	default:
		return MCP2221_ERR_INVALID;
	}

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_adc_reference_from_bits(
	uint8_t bits,
	mcp2221_analog_voltage_reference_t *reference) {
	if (!reference)
		return MCP2221_ERR_INVALID;

	if ((bits & MCP2221_ADC_REF_MASK) == MCP2221_ADC_REF_VDD) {
		*reference = MCP2221_ANALOG_VOLTAGE_REF_VDD;
		return MCP2221_ERR_OK;
	}

	switch (bits & MCP2221_ADC_VRM_MASK) {
	case MCP2221_ADC_VRM_OFF:
		*reference = MCP2221_ANALOG_VOLTAGE_REF_OFF;
		break;

	case MCP2221_ADC_VRM_1024:
		*reference = MCP2221_ANALOG_VOLTAGE_REF_1_024V;
		break;

	case MCP2221_ADC_VRM_2048:
		*reference = MCP2221_ANALOG_VOLTAGE_REF_2_048V;
		break;

	case MCP2221_ADC_VRM_4096:
		*reference = MCP2221_ANALOG_VOLTAGE_REF_4_096V;
		break;

	default:
		return MCP2221_ERR_INVALID;
	}

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_adc_raw_to_volts(
	uint16_t raw,
	double reference_voltage,
	double *volts) {
	if (!volts || raw > 1023 || !(reference_voltage > 0.0))
		return MCP2221_ERR_INVALID;

	*volts = ((double)raw / 1024.0) * reference_voltage;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_dac_reference_to_bits(
	mcp2221_analog_voltage_reference_t reference,
	int *bits) {
	if (!bits)
		return MCP2221_ERR_INVALID;

	switch (reference) {
	case MCP2221_ANALOG_VOLTAGE_REF_OFF:
		*bits = MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_OFF;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_VDD:
		*bits = MCP2221_DAC_REF_VDD | MCP2221_DAC_VRM_OFF;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_1_024V:
		*bits = MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_1024;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_2_048V:
		*bits = MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_2048;
		break;

	case MCP2221_ANALOG_VOLTAGE_REF_4_096V:
		*bits = MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_4096;
		break;

	default:
		return MCP2221_ERR_INVALID;
	}

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_get_reference_voltage(
	const mcp2221_t *dev,
	mcp2221_analog_voltage_reference_t reference,
	double *volts) {
	if (!volts)
		return MCP2221_ERR_INVALID;

	switch (reference) {
	case MCP2221_ANALOG_VOLTAGE_REF_1_024V:
		*volts = 1.024;
		return MCP2221_ERR_OK;

	case MCP2221_ANALOG_VOLTAGE_REF_2_048V:
		*volts = 2.048;
		return MCP2221_ERR_OK;

	case MCP2221_ANALOG_VOLTAGE_REF_4_096V:
		*volts = 4.096;
		return MCP2221_ERR_OK;

	case MCP2221_ANALOG_VOLTAGE_REF_VDD:
		return mcp2221_internal_analog_get_vdd(dev, volts);

	case MCP2221_ANALOG_VOLTAGE_REF_OFF:
	default:
		return MCP2221_ERR_INVALID;
	}
}

mcp2221_error_code_t mcp2221_internal_analog_state_set_vdd(
	mcp2221_internal_analog_state_t *state,
	double volts) {
	if (!state)
		return MCP2221_ERR_INVALID;

	if (!(volts >= MCP2221_MIN_VDD_VOLTS &&
	      volts <= MCP2221_MAX_VDD_VOLTS))
		return MCP2221_ERR_INVALID;

	state->vdd = volts;
	state->vdd_valid = 1;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_state_get_vdd(
	const mcp2221_internal_analog_state_t *state,
	double *volts) {
	if (!state || !volts)
		return MCP2221_ERR_INVALID;

	if (!state->vdd_valid)
		return MCP2221_ERR_INVALID;

	*volts = state->vdd;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_adc_raw_to_normalized(
	uint16_t raw,
	double *normalized) {
	if (!normalized || raw > 1023)
		return MCP2221_ERR_INVALID;

	*normalized = (double)raw / 1024.0;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_internal_analog_dac_normalized_to_raw(
	double normalized,
	uint8_t *raw) {
	if (!raw ||
	    !(normalized >= 0.0) ||
	    normalized > (31.0 / 32.0))
		return MCP2221_ERR_INVALID;

	*raw = (uint8_t)(normalized * 32.0);
	return MCP2221_ERR_OK;
}