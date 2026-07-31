#ifndef MCP2221_SRAM_H
#define MCP2221_SRAM_H

#include <stdint.h>

#include "mcp2221.h"

// Special value: -1 means "preserve existing SRAM value"
#define MCP2221_CONFIG_KEEP (-1)
/* Legacy macro alias; prefer MCP2221_CONFIG_KEEP. */
#define MCP_CONFIG_KEEP MCP2221_CONFIG_KEEP

// GPIO config struct (bits 7..0 of GPx SRAM byte)
typedef struct {
	int value;	   /* 0/1 or MCP2221_CONFIG_KEEP */
	int direction; /* 0=out, 1=in, or MCP2221_CONFIG_KEEP */
	int function;  /* GPIO_FUNC_xxx or MCP2221_CONFIG_KEEP */
} MCP_SRAM_GP_Config;
typedef MCP_SRAM_GP_Config mcp2221_sram_gp_config_t;

// Interrupt control
typedef struct {
	int pos_edge; /* enable/disable or MCP2221_CONFIG_KEEP */
	int neg_edge;
	int clear_flag; /* 0=preserve, 1=clear, MCP2221_CONFIG_KEEP */
} MCP_SRAM_INT_Config;
typedef MCP_SRAM_INT_Config mcp2221_sram_int_config_t;

// ADC reference / Vref
typedef struct {
	// Kept for backwards compatibility with earlier revisions of this C port.
	// EasyMCP2221 v1.8.4 does not expose an "alter_ref" flag; it always sends ADC/DAC ref bytes as part of SRAM_config.
	// This field is currently ignored by `mcp2221_sram_config()` and should be set to `MCP2221_CONFIG_KEEP`.
	int alter_ref; /* deprecated/ignored */
	int vrm;	   /* ADC_VRM_xxx */
	int ref_src;   /* MCP2221_ADC_REF_VRM / MCP2221_ADC_REF_VDD */
} MCP_SRAM_ADC_Config;
typedef MCP_SRAM_ADC_Config mcp2221_sram_adc_config_t;

// DAC reference
typedef struct {
	// Deprecated/ignored (see MCP_SRAM_ADC_Config.alter_ref).
	int alter_ref;
	int vrm;
	int ref_src;
} MCP_SRAM_DAC_Ref_Config;
typedef MCP_SRAM_DAC_Ref_Config mcp2221_sram_dac_ref_config_t;

// DAC new value
typedef struct {
	// Deprecated/ignored. Set to MCP2221_CONFIG_KEEP.
	int alter_value;
	int value; /* 0..31 */
} MCP_SRAM_DAC_Value_Config;
typedef MCP_SRAM_DAC_Value_Config mcp2221_sram_dac_value_config_t;

// Clock output
typedef struct {
	// Deprecated/ignored. Clock updates are driven by `duty`/`div` values.
	int alter_clk;
	int duty; /* CLK_DUTY_xx */
	int div;  /* CLK_DIV_xx */
} MCP_SRAM_Clock_Config;
typedef MCP_SRAM_Clock_Config mcp2221_sram_clock_config_t;

// Main aggregated configuration
typedef struct {
	MCP_SRAM_GP_Config gp[4];
	MCP_SRAM_INT_Config int_cfg;
	MCP_SRAM_ADC_Config adc_cfg;
	MCP_SRAM_DAC_Ref_Config dac_ref;
	MCP_SRAM_DAC_Value_Config dac_val;
	MCP_SRAM_Clock_Config clk_cfg;
} MCP2221_SRAM_Config;
typedef MCP2221_SRAM_Config mcp2221_sram_config_t;

// Apply SRAM configuration
mcp2221_error_code_t mcp2221_sram_config(mcp2221_t *dev, const mcp2221_sram_config_t *cfg);

#endif	// MCP2221_SRAM_H
