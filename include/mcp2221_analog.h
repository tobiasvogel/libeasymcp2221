/**
 * @file mcp2221_analog.h
 * @brief Analog, clock-output, and interrupt-on-change helpers.
 */

#ifndef MCP2221_ANALOG_H
#define MCP2221_ANALOG_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Store the externally supplied MCP2221 supply voltage.
 *
 * The MCP2221 cannot provide the application with a sufficiently accurate VDD
 * value for ADC/DAC conversion, so the caller supplies it explicitly. The
 * value is stored in the device context and is used whenever a voltage
 * conversion operates with VDD as its reference.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] volts Supply voltage in volts. Must be within the supported
 *                   MCP2221 supply-voltage range.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         handle or voltage, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_analog_set_vdd(
	mcp2221_t *dev,
	double volts);

/**
 * @brief Return the configured MCP2221 supply voltage.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] volts Receives the previously configured supply voltage.
 *
 * @return MCP2221_ERR_OK on success, or MCP2221_ERR_INVALID if an argument is
 *         invalid or no VDD value has been configured.
 *
 * @see mcp2221_analog_set_vdd()
 */
MCP2221_API mcp2221_error_code_t mcp2221_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts);

/**
 * @brief Configure the ADC voltage reference.
 *
 * Accepted reference strings are `"OFF"`, `"VDD"`, `"1.024V"`, `"2.048V"`,
 * and `"4.096V"`. Matching is case-insensitive.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] ref_str ADC reference selection string.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an unsupported
 *         reference or invalid argument, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_config(mcp2221_t *dev, const char *ref_str);

/**
 * @brief Read the three MCP2221 ADC channels as raw 10-bit values.
 *
 * The returned array maps channels to pins as follows:
 * - `out[0]`: ADC channel 0 on GP1
 * - `out[1]`: ADC channel 1 on GP2
 * - `out[2]`: ADC channel 2 on GP3
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] out Three-element array receiving raw values from 0 through
 *                 1023.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_read_raw(mcp2221_t *dev, uint16_t out[3]);

/**
 * @brief Read the three ADC channels as normalized values.
 *
 * Each raw 10-bit result is divided by 1024.0 to match EasyMCP2221 behavior.
 * The returned range is therefore 0.0 through 1023.0/1024.0 rather than
 * reaching exactly 1.0.
 *
 * Array index 0 through 2 corresponds to GP1 through GP3.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] out Three-element array receiving normalized ADC values.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments or ADC data, or another mcp2221_error_code_t value on
 *         failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_read_normalized(
	mcp2221_t *dev,
	double out[3]);

/**
 * @brief Read the three ADC channels as voltages.
 *
 * The current ADC reference is read from device SRAM. Fixed internal
 * references are resolved automatically. If VDD is selected, a supply voltage
 * must first have been provided with mcp2221_analog_set_vdd(). An OFF
 * reference cannot be converted to volts.
 *
 * Each raw result is converted as `raw / 1024.0 * reference_voltage`.
 * Array index 0 through 2 corresponds to GP1 through GP3.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] out Three-element array receiving ADC voltages.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID when the reference
 *         cannot be resolved or an argument/data value is invalid, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_read_volts(
	mcp2221_t *dev,
	double out[3]);

/**
 * @brief Configure the DAC voltage reference while preserving its output code.
 *
 * Accepted reference strings are `"OFF"`, `"VDD"`, `"1.024V"`, `"2.048V"`,
 * and `"4.096V"`. Matching is case-insensitive.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] ref_str DAC reference selection string.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an unsupported
 *         reference or invalid argument, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @see mcp2221_dac_config_out()
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_config(mcp2221_t *dev, const char *ref_str);

/**
 * @brief Configure the DAC voltage reference and optionally its raw output code.
 *
 * A negative @p out_code preserves the current 5-bit DAC value. Values from 0
 * through 31 set the new output code.
 *
 * When the reference changes, the helper first turns the DAC reference module
 * off and sets the output value to zero before applying the requested
 * reference and value. This preserves the EasyMCP2221 workaround for the
 * MCP2221 ADC/DAC reference-transition hardware quirk.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] ref_str DAC reference selection string. Accepted values are
 *                    `"OFF"`, `"VDD"`, `"1.024V"`, `"2.048V"`, and `"4.096V"`
 *                    (case-insensitive).
 * @param[in] out_code Raw DAC code from 0 through 31, or any negative value to
 *                     preserve the current code.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         reference, code, or argument, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_config_out(mcp2221_t *dev, const char *ref_str, int out_code);

/**
 * @brief Write a raw 5-bit DAC output code.
 *
 * The currently configured DAC voltage reference is preserved.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] code Raw DAC code from 0 through 31.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         argument or code, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_write_raw(mcp2221_t *dev, uint8_t code);

/**
 * @brief Write a normalized DAC output value.
 *
 * The value is multiplied by 32 and converted to the 5-bit DAC code. Accepted
 * inputs range from 0.0 through 31.0/32.0 inclusive. Values outside that
 * range, including NaN, are rejected.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] normalized Normalized DAC output value.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         argument or normalized value, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_write_normalized(
	mcp2221_t *dev,
	double normalized);

/**
 * @brief Write a DAC output voltage.
 *
 * The current DAC reference is read from device SRAM. Fixed internal
 * references are resolved automatically. If VDD is selected, a supply voltage
 * must first have been provided with mcp2221_analog_set_vdd(). An OFF
 * reference cannot be converted to a voltage.
 *
 * The largest accepted voltage is 31.0/32.0 of the selected reference voltage.
 * Values between two DAC steps are truncated to the lower raw output code.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] volts Requested DAC output voltage.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         voltage, unresolved reference, or invalid argument, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_write_volts(
	mcp2221_t *dev,
	double volts);

/**
 * @brief Configure the MCP2221 clock output.
 *
 * Supported duty-cycle values are 0, 25, 50, and 75 percent. Supported
 * frequency strings are `"375kHz"`, `"750kHz"`, `"1.5MHz"`, `"3MHz"`,
 * `"6MHz"`, `"12MHz"`, and `"24MHz"`. Frequency matching is case-insensitive.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] duty_percent Clock duty cycle in percent.
 * @param[in] freq_str Clock frequency selection string.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an unsupported
 *         duty cycle, frequency, or invalid argument, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_clock_config(mcp2221_t *dev, int duty_percent, const char *freq_str);

/**
 * @brief Read the interrupt-on-change flag.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] flag Receives the interrupt-on-change flag reported by the
 *                  MCP2221.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_ioc_read(mcp2221_t *dev, uint8_t *flag);

/**
 * @brief Clear the interrupt-on-change flag.
 *
 * @param[in] dev Open MCP2221 device handle.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         device handle, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_ioc_clear(mcp2221_t *dev);

/**
 * @brief Configure interrupt-on-change edge detection.
 *
 * Accepted edge strings are `"none"`, `"rising"`, `"falling"`, and `"both"`.
 * Matching is case-insensitive.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] edge Requested edge-detection mode.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an unsupported
 *         mode or invalid argument, or another mcp2221_error_code_t value on
 *         failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_ioc_config(mcp2221_t *dev, const char *edge);

MCP2221_END_DECLS
#endif	// MCP2221_ANALOG_H
