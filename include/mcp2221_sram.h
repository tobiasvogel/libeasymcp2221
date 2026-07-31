#ifndef MCP2221_SRAM_H
#define MCP2221_SRAM_H

#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

// Special value: -1 means "preserve existing SRAM value"
#define MCP2221_CONFIG_KEEP (-1)

// GPIO config struct (bits 7..0 of GPx SRAM byte)
typedef struct {
	int value;	   /* 0/1 or MCP2221_CONFIG_KEEP */
	int direction; /* 0=out, 1=in, or MCP2221_CONFIG_KEEP */
	int function;  /* MCP2221_GPIO_FUNC_* or MCP2221_CONFIG_KEEP */
} mcp2221_sram_gp_config_t;

// Interrupt control
typedef struct {
	int pos_edge; /* enable/disable or MCP2221_CONFIG_KEEP */
	int neg_edge;
	int clear_flag; /* 0=preserve, 1=clear, MCP2221_CONFIG_KEEP */
} mcp2221_sram_int_config_t;

// ADC reference / Vref
typedef struct {
	int vrm;	   /* MCP2221_ADC_VRM_* or MCP2221_CONFIG_KEEP */
	int ref_src;   /* MCP2221_ADC_REF_VRM / MCP2221_ADC_REF_VDD */
} mcp2221_sram_adc_config_t;

// DAC reference
typedef struct {
	int vrm;
	int ref_src;
} mcp2221_sram_dac_ref_config_t;

// DAC new value
typedef struct {
	int value; /* 0..31 */
} mcp2221_sram_dac_value_config_t;

// Clock output
typedef struct {
	int duty; /* MCP2221_CLK_DUTY_* or MCP2221_CONFIG_KEEP */
	int div;  /* MCP2221_CLK_DIV_* or MCP2221_CONFIG_KEEP */
} mcp2221_sram_clock_config_t;

// Main aggregated configuration
typedef struct {
	mcp2221_sram_gp_config_t gp[4];
	mcp2221_sram_int_config_t int_cfg;
	mcp2221_sram_adc_config_t adc_cfg;
	mcp2221_sram_dac_ref_config_t dac_ref;
	mcp2221_sram_dac_value_config_t dac_val;
	mcp2221_sram_clock_config_t clk_cfg;
} mcp2221_sram_config_t;

// Apply SRAM configuration
MCP2221_API mcp2221_error_code_t mcp2221_sram_config(mcp2221_t *dev, const mcp2221_sram_config_t *cfg);

MCP2221_END_DECLS
#endif	// MCP2221_SRAM_H
