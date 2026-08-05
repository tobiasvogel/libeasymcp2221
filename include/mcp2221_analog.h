#ifndef MCP2221_ANALOG_H
#define MCP2221_ANALOG_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/* ----------------- Analog ----------------- */

/**
 * Set the externally supplied MCP2221 supply voltage.
 *
 * This value is used when ADC or DAC voltage conversion uses VDD as its
 * reference. Valid values are in the supported MCP2221 supply range.
 *
 * @param dev Device handle
 * @param volts Supply voltage in volts
 * @return MCP2221_ERR_OK on success or another error code on failure
 */
MCP2221_API mcp2221_error_code_t mcp2221_analog_set_vdd(
	mcp2221_t *dev,
	double volts);

/**
 * Return the configured MCP2221 supply voltage.
 *
 * @param dev Device handle
 * @param volts Output pointer receiving the configured voltage
 * @return MCP2221_ERR_OK on success or MCP2221_ERR_INVALID if no VDD value
 *         has been configured
 */
MCP2221_API mcp2221_error_code_t mcp2221_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts);

/* ----------------- ADC ----------------- */

/**
 * Configure ADC reference.
 *
 * ref_str:
 *   "OFF"
 *   "1.024V"
 *   "2.048V"
 *   "4.096V"
 *   "VDD"
 *
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on error.
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_config(mcp2221_t *dev, const char *ref_str);

/**
 * Read all three ADC channels (GP1, GP2, GP3) as raw 0..1023.
 *
 * out[0] = CH0 (GP1)
 * out[1] = CH1 (GP2)
 * out[2] = CH2 (GP3)
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_read_raw(mcp2221_t *dev, uint16_t out[3]);

/**
 * Read all three ADC channels as normalized values.
 *
 * Values are normalized to the range 0.0 to approximately 1.0 following the
 * EasyMCP2221 convention. The raw 10-bit ADC result is divided by 1024.0,
 * therefore the maximum returned value is approximately 0.999 rather than
 * exactly 1.0.
 *
 * out[0] = CH0 (GP1)
 * out[1] = CH1 (GP2)
 * out[2] = CH2 (GP3)
 *
 * @param dev Device handle
 * @param out Output array receiving the three normalized ADC values
 * @return MCP2221_ERR_OK on success or another error code on failure
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_read_normalized(
	mcp2221_t *dev,
	double out[3]);

/**
 * Read all three ADC channels as voltages.
 *
 * The currently configured ADC reference is read from device SRAM. Internal
 * references are resolved automatically. When VDD is selected, the supply
 * voltage must first be configured with mcp2221_analog_set_vdd().
 *
 * Values follow the EasyMCP2221 conversion convention: the raw 10-bit ADC
 * result is divided by 1024.0 and multiplied by the reference voltage.
 *
 * out[0] = CH0 (GP1)
 * out[1] = CH1 (GP2)
 * out[2] = CH2 (GP3)
 *
 * @param dev Device handle
 * @param out Output array receiving the three voltages
 * @return MCP2221_ERR_OK on success or another error code on failure
 */
MCP2221_API mcp2221_error_code_t mcp2221_adc_read_volts(
	mcp2221_t *dev,
	double out[3]);

/* ----------------- DAC ----------------- */

/**
 * Configure DAC reference.
 *
 * ref_str:
 *   "OFF"
 *   "1.024V"
 *   "2.048V"
 *   "4.096V"
 *   "VDD"
 *
 * Corresponds to Python DAC_config(ref=...).
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_config(mcp2221_t *dev, const char *ref_str);

/**
 * Configure DAC reference and optionally the output code (0..31).
 *
 * If `out_code` is negative, the current DAC value is preserved (like Python's `out=None`).
 * If `out_code` is 0..31, it sets that code (like Python's `out=<value>`).
 *
 * Mirrors EasyMCP2221.DAC_config(ref=..., out=...): turns DAC off before changing ref to avoid VRM crash,
 * then applies desired ref and value.
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_config_out(mcp2221_t *dev, const char *ref_str, int out_code);

/**
 * Write raw DAC code (0..31).
 *
 * Corresponds to Python DAC_write(out) using the raw value.
 */
MCP2221_API mcp2221_error_code_t mcp2221_dac_write_raw(mcp2221_t *dev, uint8_t code);

// Clock output
/**
 * Configure clock output frequency and duty cycle.
 *
 * duty_percent: 0, 25, 50, 75
 * freq_str: "375kHz", "750kHz", "1.5MHz", "3MHz", "6MHz", "12MHz", "24MHz"
 */
MCP2221_API mcp2221_error_code_t mcp2221_clock_config(mcp2221_t *dev, int duty_percent, const char *freq_str);

// Interrupt On Change (IOC)

/** Read IOC flag (0/1). */
MCP2221_API mcp2221_error_code_t mcp2221_ioc_read(mcp2221_t *dev, uint8_t *flag);

/** Clear IOC flag. */
MCP2221_API mcp2221_error_code_t mcp2221_ioc_clear(mcp2221_t *dev);

/**
 * Configure IOC edge detection.
 *
 * edge:
 *   "none"
 *   "rising"
 *   "falling"
 *   "both"
 */
MCP2221_API mcp2221_error_code_t mcp2221_ioc_config(mcp2221_t *dev, const char *edge);

MCP2221_END_DECLS
#endif	// MCP2221_ANALOG_H
