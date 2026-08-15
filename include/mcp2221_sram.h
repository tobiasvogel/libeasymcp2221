/**
 * @file mcp2221_sram.h
 * @brief Runtime SRAM configuration for GPIO, analog references, clock, and IOC.
 */

#ifndef MCP2221_SRAM_H
#define MCP2221_SRAM_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Preserve the current value of an SRAM configuration field.
 *
 * Use this sentinel for fields in mcp2221_sram_config_t that should remain
 * unchanged.
 *
 * @note This sentinel is specific to the SRAM configuration API. GPIO output
 *       updates use MCP2221_GPIO_KEEP instead.
 */
#define MCP2221_CONFIG_KEEP (-1)

/**
 * @brief SRAM configuration for one MCP2221 GP pin.
 *
 * The three fields correspond to the output value, GPIO direction, and pin
 * function encoded in the MCP2221 GP SRAM byte. Each field can be changed
 * independently or preserved with MCP2221_CONFIG_KEEP.
 */
typedef struct {
	/**
	 * @brief GPIO output value.
	 *
	 * Use 0 for low, 1 for high, or MCP2221_CONFIG_KEEP to preserve the
	 * current value.
	 */
	int value;

	/**
	 * @brief GPIO direction.
	 *
	 * Use MCP2221_DIR_OUTPUT (0), MCP2221_DIR_INPUT (1), or
	 * MCP2221_CONFIG_KEEP.
	 */
	int direction;

	/**
	 * @brief Pin-function selector.
	 *
	 * Use an MCP2221_GPIO_FUNC_* value valid for the corresponding GP pin, or
	 * MCP2221_CONFIG_KEEP. Supported selectors are:
	 * - GP0: GPIO, DEDICATED, ALT_0
	 * - GP1: GPIO, DEDICATED, ALT_0, ALT_1, ALT_2
	 * - GP2: GPIO, DEDICATED, ALT_0, ALT_1
	 * - GP3: GPIO, DEDICATED, ALT_0, ALT_1
	 */
	int function;
} mcp2221_sram_gp_config_t;

/**
 * @brief Interrupt-on-change SRAM configuration.
 */
typedef struct {
	/**
	 * @brief Positive-edge detection setting.
	 *
	 * Use 0 to disable, 1 to enable, or MCP2221_CONFIG_KEEP to preserve the
	 * current setting.
	 */
	int pos_edge;

	/**
	 * @brief Negative-edge detection setting.
	 *
	 * Use 0 to disable, 1 to enable, or MCP2221_CONFIG_KEEP to preserve the
	 * current setting.
	 */
	int neg_edge;

	/**
	 * @brief Interrupt-flag clear control.
	 *
	 * Use 1 to request clearing the interrupt flag, 0 to leave the clear bit
	 * deasserted, or MCP2221_CONFIG_KEEP to preserve the current field.
	 */
	int clear_flag;
} mcp2221_sram_int_config_t;

/**
 * @brief ADC voltage-reference SRAM configuration.
 */
typedef struct {
	/**
	 * @brief Internal voltage-reference-module selection.
	 *
	 * Use MCP2221_ADC_VRM_OFF, MCP2221_ADC_VRM_1024,
	 * MCP2221_ADC_VRM_2048, MCP2221_ADC_VRM_4096, or
	 * MCP2221_CONFIG_KEEP.
	 */
	int vrm;

	/**
	 * @brief ADC reference source.
	 *
	 * Use MCP2221_ADC_REF_VRM for the internal voltage-reference module,
	 * MCP2221_ADC_REF_VDD for VDD, or MCP2221_CONFIG_KEEP.
	 */
	int ref_src;
} mcp2221_sram_adc_config_t;

/**
 * @brief DAC voltage-reference SRAM configuration.
 */
typedef struct {
	/**
	 * @brief Internal voltage-reference-module selection.
	 *
	 * Use MCP2221_DAC_VRM_OFF, MCP2221_DAC_VRM_1024,
	 * MCP2221_DAC_VRM_2048, MCP2221_DAC_VRM_4096, or
	 * MCP2221_CONFIG_KEEP.
	 */
	int vrm;

	/**
	 * @brief DAC reference source.
	 *
	 * Use MCP2221_DAC_REF_VRM for the internal voltage-reference module,
	 * MCP2221_DAC_REF_VDD for VDD, or MCP2221_CONFIG_KEEP.
	 */
	int ref_src;
} mcp2221_sram_dac_ref_config_t;

/**
 * @brief DAC output-code SRAM configuration.
 */
typedef struct {
	/**
	 * @brief Raw 5-bit DAC output code.
	 *
	 * Use a value from 0 through 31, or MCP2221_CONFIG_KEEP to preserve the
	 * current DAC value.
	 */
	int value;
} mcp2221_sram_dac_value_config_t;

/**
 * @brief Clock-output SRAM configuration.
 */
typedef struct {
	/**
	 * @brief Clock-output duty-cycle selector.
	 *
	 * Use MCP2221_CLK_DUTY_0, MCP2221_CLK_DUTY_25,
	 * MCP2221_CLK_DUTY_50, MCP2221_CLK_DUTY_75, or
	 * MCP2221_CONFIG_KEEP.
	 */
	int duty;

	/**
	 * @brief Clock-output divider selector.
	 *
	 * Use MCP2221_CLK_DIV_1 through MCP2221_CLK_DIV_7, or
	 * MCP2221_CONFIG_KEEP. The MCP2221_CLK_FREQ_* aliases may be used for the
	 * corresponding output frequencies.
	 */
	int div;
} mcp2221_sram_clock_config_t;

/**
 * @brief Aggregated runtime SRAM configuration.
 *
 * Each nested field may be changed independently. Set fields that should
 * remain unchanged to MCP2221_CONFIG_KEEP.
 *
 * Before writing, mcp2221_sram_config() reads the current SRAM state and
 * merges the requested changes with existing values. The configuration affects
 * runtime SRAM only; it does not write persistent flash settings.
 */
typedef struct {
	/**
	 * @brief GP0 through GP3 configuration.
	 *
	 * Array index 0 through 3 corresponds to GP0 through GP3.
	 */
	mcp2221_sram_gp_config_t gp[4];

	/** @brief Interrupt-on-change configuration. */
	mcp2221_sram_int_config_t int_cfg;

	/** @brief ADC reference configuration. */
	mcp2221_sram_adc_config_t adc_cfg;

	/** @brief DAC reference configuration. */
	mcp2221_sram_dac_ref_config_t dac_ref;

	/** @brief DAC output-code configuration. */
	mcp2221_sram_dac_value_config_t dac_val;

	/** @brief Clock-output configuration. */
	mcp2221_sram_clock_config_t clk_cfg;
} mcp2221_sram_config_t;

/**
 * @brief Apply a runtime SRAM configuration.
 *
 * The function validates all fields, reads the current device SRAM state, and
 * applies the requested changes while preserving fields set to
 * MCP2221_CONFIG_KEEP.
 *
 * GPIO configuration is merged with the library's cached GPIO state when
 * available so that output changes made through the GPIO API are preserved.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] cfg SRAM configuration to apply. Must not be `NULL`.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for an invalid
 *         argument or unsupported field value, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @note This function changes runtime SRAM settings only. It does not modify
 *       persistent MCP2221 flash configuration.
 */
MCP2221_API mcp2221_error_code_t mcp2221_sram_config(mcp2221_t *dev, const mcp2221_sram_config_t *cfg);

MCP2221_END_DECLS
#endif	// MCP2221_SRAM_H
