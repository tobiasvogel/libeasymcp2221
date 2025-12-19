#include "mcp2221_pin.h"

#include <string.h>

#include "constants.h"
#include "mcp2221_sram.h"

static int is_function_allowed(MCP_GPIO_Pin pin, MCP_PinFunction function) {
	if (function == MCP_PIN_FUNC_KEEP)
		return 1;
	switch (pin) {
		case MCP_GP0:
			return function == MCP_PIN_FUNC_GPIO_IN || function == MCP_PIN_FUNC_GPIO_OUT ||
				   function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0;
		case MCP_GP1:
			return function == MCP_PIN_FUNC_GPIO_IN || function == MCP_PIN_FUNC_GPIO_OUT ||
				   function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0 || function == MCP_PIN_FUNC_ALT1 ||
				   function == MCP_PIN_FUNC_ALT2;
		case MCP_GP2:
			return function == MCP_PIN_FUNC_GPIO_IN || function == MCP_PIN_FUNC_GPIO_OUT ||
				   function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0 || function == MCP_PIN_FUNC_ALT1;
		case MCP_GP3:
			return function == MCP_PIN_FUNC_GPIO_IN || function == MCP_PIN_FUNC_GPIO_OUT ||
				   function == MCP_PIN_FUNC_DEDICATED || function == MCP_PIN_FUNC_ALT0 || function == MCP_PIN_FUNC_ALT1;
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

	// Python Device.set_pin_function sets full GP byte (function + direction + outval).
	// We mirror that as close as possible; since we don't have an 'out' parameter here,
	// we default output value to 0 (Python default outX=False).
	switch (function) {
		case MCP_PIN_FUNC_GPIO_IN:
			cfg.gp[pin].function = GPIO_FUNC_GPIO;
			cfg.gp[pin].direction = DIR_INPUT;
			cfg.gp[pin].value = 0;
			break;
		case MCP_PIN_FUNC_GPIO_OUT:
			cfg.gp[pin].function = GPIO_FUNC_GPIO;
			cfg.gp[pin].direction = DIR_OUTPUT;
			cfg.gp[pin].value = 0;
			break;
		case MCP_PIN_FUNC_DEDICATED:
			cfg.gp[pin].function = GPIO_FUNC_DEDICATED;
			cfg.gp[pin].direction = DIR_OUTPUT;
			cfg.gp[pin].value = 0;
			break;
		case MCP_PIN_FUNC_ALT0:
			cfg.gp[pin].function = GPIO_FUNC_ALT_0;
			cfg.gp[pin].direction = DIR_OUTPUT;
			cfg.gp[pin].value = 0;
			break;
		case MCP_PIN_FUNC_ALT1:
			cfg.gp[pin].function = GPIO_FUNC_ALT_1;
			cfg.gp[pin].direction = DIR_OUTPUT;
			cfg.gp[pin].value = 0;
			break;
		case MCP_PIN_FUNC_ALT2:
			cfg.gp[pin].function = GPIO_FUNC_ALT_2;
			cfg.gp[pin].direction = DIR_OUTPUT;
			cfg.gp[pin].value = 0;
			break;
		default:
			return MCP_ERR_INVALID;
	}

	return mcp2221_sram_config(dev, &cfg);
}

static int fill_gp_config_from_function(MCP_GPIO_Pin pin, MCP_PinFunction function, int out_value,
										MCP_SRAM_GP_Config *out_gp) {
	if (!out_gp)
		return MCP_ERR_INVALID;

	// Mirror Python constraint: out=True only valid if gpX == GPIO_OUT (and gpX must not be None/KEEP).
	if (out_value == 1 && function != MCP_PIN_FUNC_GPIO_OUT)
		return MCP_ERR_INVALID;

	// If we are going to change the pin, set all 3 fields explicitly (Python sets a full gp byte).
	out_gp->value = 0;
	out_gp->direction = DIR_OUTPUT;
	out_gp->function = MCP_CONFIG_KEEP;

	switch (function) {
		case MCP_PIN_FUNC_KEEP:
			out_gp->value = MCP_CONFIG_KEEP;
			out_gp->direction = MCP_CONFIG_KEEP;
			out_gp->function = MCP_CONFIG_KEEP;
			return MCP_ERR_OK;

		case MCP_PIN_FUNC_GPIO_IN:
			out_gp->function = GPIO_FUNC_GPIO;
			out_gp->direction = DIR_INPUT;
			out_gp->value = 0;
			return MCP_ERR_OK;

		case MCP_PIN_FUNC_GPIO_OUT:
			out_gp->function = GPIO_FUNC_GPIO;
			out_gp->direction = DIR_OUTPUT;
			out_gp->value = out_value ? 1 : 0;
			return MCP_ERR_OK;

		case MCP_PIN_FUNC_DEDICATED:
			out_gp->function = GPIO_FUNC_DEDICATED;
			// Python does not set DIR bits for non-GPIO functions; leave as output (0) with value 0.
			out_gp->direction = DIR_OUTPUT;
			out_gp->value = 0;
			return MCP_ERR_OK;

		case MCP_PIN_FUNC_ALT0:
			out_gp->function = GPIO_FUNC_ALT_0;
			out_gp->direction = DIR_OUTPUT;
			out_gp->value = 0;
			return MCP_ERR_OK;

		case MCP_PIN_FUNC_ALT1:
			out_gp->function = GPIO_FUNC_ALT_1;
			out_gp->direction = DIR_OUTPUT;
			out_gp->value = 0;
			return MCP_ERR_OK;

		case MCP_PIN_FUNC_ALT2:
			// Only valid on GP1 (IOC) in EasyMCP2221
			if (pin != MCP_GP1)
				return MCP_ERR_INVALID;
			out_gp->function = GPIO_FUNC_ALT_2;
			out_gp->direction = DIR_OUTPUT;
			out_gp->value = 0;
			return MCP_ERR_OK;

		default:
			return MCP_ERR_INVALID;
	}
}

int mcp2221_set_pin_functions(MCP2221 *dev, const MCP2221_PinFunctions *cfg) {
	if (!dev || !cfg)
		return MCP_ERR_INVALID;

	MCP2221_SRAM_Config sram;
	memset(&sram, 0, sizeof(sram));

	for (int i = 0; i < 4; i++) {
		sram.gp[i].value = MCP_CONFIG_KEEP;
		sram.gp[i].direction = MCP_CONFIG_KEEP;
		sram.gp[i].function = MCP_CONFIG_KEEP;
	}

	// No changes for other SRAM fields
	sram.int_cfg.pos_edge = MCP_CONFIG_KEEP;
	sram.int_cfg.neg_edge = MCP_CONFIG_KEEP;
	sram.int_cfg.clear_flag = MCP_CONFIG_KEEP;

	sram.adc_cfg.alter_ref = MCP_CONFIG_KEEP;
	sram.adc_cfg.vrm = MCP_CONFIG_KEEP;
	sram.adc_cfg.ref_src = MCP_CONFIG_KEEP;

	sram.dac_ref.alter_ref = MCP_CONFIG_KEEP;
	sram.dac_ref.vrm = MCP_CONFIG_KEEP;
	sram.dac_ref.ref_src = MCP_CONFIG_KEEP;

	sram.dac_val.alter_value = MCP_CONFIG_KEEP;
	sram.dac_val.value = MCP_CONFIG_KEEP;

	sram.clk_cfg.alter_clk = MCP_CONFIG_KEEP;
	sram.clk_cfg.duty = MCP_CONFIG_KEEP;
	sram.clk_cfg.div = MCP_CONFIG_KEEP;

	// Validate and map each pin.
	for (int i = 0; i < 4; i++) {
		MCP_PinFunction fn = cfg->gp[i];
		if (!is_function_allowed((MCP_GPIO_Pin)i, fn))
			return MCP_ERR_INVALID;

		int outv = cfg->out[i] ? 1 : 0;
		int r = fill_gp_config_from_function((MCP_GPIO_Pin)i, fn, outv, &sram.gp[i]);
		if (r != MCP_ERR_OK)
			return r;
	}

	return mcp2221_sram_config(dev, &sram);
}
