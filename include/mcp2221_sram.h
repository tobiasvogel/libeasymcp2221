#ifndef MCP2221_SRAM_H
#define MCP2221_SRAM_H

#include <stdint.h>

#include "mcp2221.h"

// Special value: -1 means "preserve existing SRAM value"
#define MCP_CONFIG_KEEP (-1)

// GPIO config struct (bits 7..0 of GPx SRAM byte)
typedef struct {
	int value;	   /* 0/1 or MCP_CONFIG_KEEP */
	int direction; /* 0=out, 1=in, or MCP_CONFIG_KEEP */
	int function;  /* GPIO_FUNC_xxx or MCP_CONFIG_KEEP */
} MCP_SRAM_GP_Config;

// Interrupt control
typedef struct {
	int pos_edge; /* enable/disable or MCP_CONFIG_KEEP */
	int neg_edge;
	int clear_flag; /* 0=preserve, 1=clear, MCP_CONFIG_KEEP */
} MCP_SRAM_INT_Config;

// ADC reference / Vref
typedef struct {
	int alter_ref; /* FALLBACK: MCP_CONFIG_KEEP */
	int vrm;	   /* ADC_VRM_xxx */
	int ref_src;   /* ADC_REF_VRM / ADC_REF_VDD */
} MCP_SRAM_ADC_Config;

// DAC reference
typedef struct {
	int alter_ref;
	int vrm;
	int ref_src;
} MCP_SRAM_DAC_Ref_Config;

// DAC new value
typedef struct {
	int alter_value;
	int value; /* 0..31 */
} MCP_SRAM_DAC_Value_Config;

// Clock output
typedef struct {
	int alter_clk;
	int duty; /* CLK_DUTY_xx */
	int div;  /* CLK_DIV_xx */
} MCP_SRAM_Clock_Config;

// Main aggregated configuration
typedef struct {
	MCP_SRAM_GP_Config gp[4];
	MCP_SRAM_INT_Config int_cfg;
	MCP_SRAM_ADC_Config adc_cfg;
	MCP_SRAM_DAC_Ref_Config dac_ref;
	MCP_SRAM_DAC_Value_Config dac_val;
	MCP_SRAM_Clock_Config clk_cfg;
} MCP2221_SRAM_Config;

// Apply SRAM configuration
int mcp2221_sram_config(MCP2221 *dev, const MCP2221_SRAM_Config *cfg);

#endif	// MCP_SRAM_H