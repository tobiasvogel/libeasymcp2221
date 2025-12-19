#ifndef MCP2221_ANALOG_H
#define MCP2221_ANALOG_H

#include <stdint.h>

#include "mcp2221.h"

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
 * Gibt 0 bei Erfolg, <0 bei Fehler.
 */
int mcp2221_adc_config(MCP2221 *dev, const char *ref_str);

/**
 * Read all three ADC channels (GP1, GP2, GP3) as raw 0..1023.
 *
 * out[0] = CH0 (GP1)
 * out[1] = CH1 (GP2)
 * out[2] = CH2 (GP3)
 */
int mcp2221_adc_read_raw(MCP2221 *dev, uint16_t out[3]);

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
 * corresponds to Python DAC_config(ref=...),
 */
int mcp2221_dac_config(MCP2221 *dev, const char *ref_str);

/**
 * Configure DAC reference and optionally the output code (0..31).
 *
 * If `out_code` is negative, the current DAC value is preserved (like Python's `out=None`).
 * If `out_code` is 0..31, it sets that code (like Python's `out=<value>`).
 *
 * Mirrors EasyMCP2221.DAC_config(ref=..., out=...): turns DAC off before changing ref to avoid VRM crash,
 * then applies desired ref and value.
 */
int mcp2221_dac_config_out(MCP2221 *dev, const char *ref_str, int out_code);

/**
 * Write raw DAC code (0..31).
 *
 * corresponds to Python DAC_write(out) using "raw"-value.
 */
int mcp2221_dac_write_raw(MCP2221 *dev, uint8_t code);

// Clock output
/**
 * Configure clock output frequency and duty cycle.
 *
 * duty_percent: 0, 25, 50, 75
 * freq_str: "375kHz", "750kHz", "1.5MHz", "3MHz", "6MHz", "12MHz", "24MHz"
 */
int mcp2221_clock_config(MCP2221 *dev, int duty_percent, const char *freq_str);

// Interrupt On Change (IOC)

/** Read IOC flag (0/1). */
int mcp2221_ioc_read(MCP2221 *dev, uint8_t *flag);

/** Clear IOC flag. */
int mcp2221_ioc_clear(MCP2221 *dev);

/**
 * Configure IOC edge detection.
 *
 * edge:
 *   "none"
 *   "rising"
 *   "falling"
 *   "both"
 */
int mcp2221_ioc_config(MCP2221 *dev, const char *edge);

#endif	// MCP2221_ANALOG_H
