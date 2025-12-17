#include "mcp2221_pin.h"

#include <string.h>

#include "constants.h"
#include "mcp2221_sram.h"

static int is_function_allowed(MCP_GPIO_Pin pin, MCP_PinFunction function) {
	switch (pin) {
		case MCP_GP0:
			return function == MCP_PIN_FUNC_GPIO || function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0;
		case MCP_GP1:
			return function == MCP_PIN_FUNC_GPIO || function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0 ||
				   function == MCP_PIN_FUNC_ALT1 || function == MCP_PIN_FUNC_ALT2;
		case MCP_GP2:
			return function == MCP_PIN_FUNC_GPIO || function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0 ||
				   function == MCP_PIN_FUNC_ALT1;
		case MCP_GP3:
			return function == MCP_PIN_FUNC_GPIO || function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0 ||
				   function == MCP_PIN_FUNC_ALT1;
		default:
			return 0;
	}
}

int mcp2221_set_pin_function(MCP2221 *dev, MCP_GPIO_Pin pin, MCP_PinFunction function) {
	if (!dev)
		return MCP_ERR_INVALID;
	if (pin < 0 || pin > 3)
		return MCP_ERR_INVALID;
	if (!is_function_allowed(pin, function))
		return MCP_ERR_INVALID;

	MCP2221_SRAM_Config cfg;
	memset(&cfg, 0, sizeof(cfg));

	for (int i = 0; i < 4; i++) {
		cfg.gp[i].value = MCP_CONFIG_KEEP;
		cfg.gp[i].direction = MCP_CONFIG_KEEP;
		cfg.gp[i].function = MCP_CONFIG_KEEP;
	}
	cfg.int_cfg.pos_edge = MCP_CONFIG_KEEP;
	cfg.int_cfg.neg_edge = MCP_CONFIG_KEEP;
	cfg.int_cfg.clear_flag = MCP_CONFIG_KEEP;

	cfg.adc_cfg.alter_ref = MCP_CONFIG_KEEP;
	cfg.adc_cfg.vrm = MCP_CONFIG_KEEP;
	cfg.adc_cfg.ref_src = MCP_CONFIG_KEEP;

	cfg.dac_ref.alter_ref = MCP_CONFIG_KEEP;
	cfg.dac_ref.vrm = MCP_CONFIG_KEEP;
	cfg.dac_ref.ref_src = MCP_CONFIG_KEEP;

	cfg.dac_val.alter_value = MCP_CONFIG_KEEP;
	cfg.dac_val.value = MCP_CONFIG_KEEP;

	cfg.clk_cfg.alter_clk = MCP_CONFIG_KEEP;
	cfg.clk_cfg.duty = MCP_CONFIG_KEEP;
	cfg.clk_cfg.div = MCP_CONFIG_KEEP;

	// Set only the function bits; keep direction/value as-is.
	cfg.gp[pin].function = function;

	return mcp2221_sram_config(dev, &cfg);
}
