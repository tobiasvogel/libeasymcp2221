#ifndef MCP2221_INTERNAL_ANALOG_H
#define MCP2221_INTERNAL_ANALOG_H

/**
 * @file mcp2221_internal_analog.h
 * @brief Internal analog helpers for libeasymcp2221.
 *
 * This header is private to the library and must not be installed or used by
 * applications. It contains the shared ADC/DAC voltage-reference model and
 * accessors for analog state stored in the opaque MCP2221 device handle.
 */

#include <stdint.h>

#include "mcp2221.h"
#include "mcp2221_error_codes.h"

MCP2221_BEGIN_DECLS

/**
 * Semantic representation of an ADC or DAC voltage reference.
 *
 * This type deliberately represents the logical reference selection rather
 * than the corresponding MCP2221 register bits. ADC and DAC register
 * encodings are derived separately.
 */
typedef enum {
	MCP2221_ANALOG_VOLTAGE_REF_OFF,
	MCP2221_ANALOG_VOLTAGE_REF_VDD,
	MCP2221_ANALOG_VOLTAGE_REF_1_024V,
	MCP2221_ANALOG_VOLTAGE_REF_2_048V,
	MCP2221_ANALOG_VOLTAGE_REF_4_096V
} mcp2221_analog_voltage_reference_t;

typedef struct {
	double vdd;
	int vdd_valid;
} mcp2221_internal_analog_state_t;

/**
 * Parse a public API reference string.
 *
 * Accepted strings are "OFF", "VDD", "1.024V", "2.048V" and "4.096V".
 * Matching is case-insensitive.
 */
mcp2221_error_code_t mcp2221_internal_analog_parse_voltage_reference(
	const char *ref_str,
	mcp2221_analog_voltage_reference_t *reference);

/**
 * Convert a semantic voltage reference to ADC SRAM register bits.
 */
mcp2221_error_code_t mcp2221_internal_analog_adc_reference_to_bits(
	mcp2221_analog_voltage_reference_t reference,
	int *bits);

/**
 * Decode ADC SRAM register bits into a semantic voltage reference.
 *
 * When VDD is selected, the VRM selection bits are ignored because they are
 * not used by the device and may still contain a previous setting.
 *
 * @param bits ADC reference bits read from SRAM
 * @param reference Output pointer receiving the decoded reference
 * @return MCP2221_ERR_OK on success or MCP2221_ERR_INVALID for invalid input
 */
mcp2221_error_code_t mcp2221_internal_analog_adc_reference_from_bits(
	uint8_t bits,
	mcp2221_analog_voltage_reference_t *reference);

/**
 * Convert a semantic voltage reference to DAC SRAM register bits.
 */
mcp2221_error_code_t mcp2221_internal_analog_dac_reference_to_bits(
	mcp2221_analog_voltage_reference_t reference,
	int *bits);

/**
 * Store the externally supplied device supply voltage.
 *
 * The MCP2221 cannot measure its own VDD accurately for this purpose, so the
 * value is supplied by the application and stored in the opaque device handle.
 */
mcp2221_error_code_t mcp2221_internal_analog_set_vdd(
	mcp2221_t *dev,
	double volts);

/**
 * Return the stored device supply voltage.
 *
 * Returns MCP2221_ERR_INVALID if no VDD value has been configured.
 */
mcp2221_error_code_t mcp2221_internal_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts);

/**
 * Resolve a semantic reference selection to its voltage in volts.
 *
 * Fixed internal references resolve directly. VDD resolves to the value
 * previously stored with mcp2221_internal_analog_set_vdd(). The OFF reference
 * and an unset VDD reference return MCP2221_ERR_INVALID.
 */
mcp2221_error_code_t mcp2221_internal_analog_get_reference_voltage(
	const mcp2221_t *dev,
	mcp2221_analog_voltage_reference_t reference,
	double *volts);

mcp2221_error_code_t mcp2221_internal_analog_state_set_vdd(
	mcp2221_internal_analog_state_t *state,
	double volts);

mcp2221_error_code_t mcp2221_internal_analog_state_get_vdd(
	const mcp2221_internal_analog_state_t *state,
	double *volts);

/**
 * Convert a raw 10-bit ADC result to the normalized EasyMCP2221 value.
 *
 * The raw value is divided by 1024.0. Consequently, the maximum raw value
 * 1023 converts to approximately 0.999 rather than exactly 1.0.
 *
 * @param raw Raw ADC result in the range 0..1023
 * @param normalized Output pointer receiving the normalized value
 * @return MCP2221_ERR_OK on success or MCP2221_ERR_INVALID for invalid input
 */
mcp2221_error_code_t mcp2221_internal_analog_adc_raw_to_normalized(
	uint16_t raw,
	double *normalized);

MCP2221_END_DECLS

#endif // MCP2221_INTERNAL_ANALOG_H