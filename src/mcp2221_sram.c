#include "mcp2221_internal.h"
#include "mcp2221_sram.h"
#include <string.h>

#include "mcp2221_internal_constants.h"
#include "mcp2221_errors.h"

// Internal helpers implemented in src/mcp2221.c (not part of the public API)

static int is_keep_or_bool(int value) {
	return value == MCP2221_CONFIG_KEEP || value == 0 || value == 1;
}

static int is_valid_gpio_function(int pin, int function) {
	if (function == MCP2221_CONFIG_KEEP)
		return 1;

	switch (pin) {
		case MCP2221_GPIO_GP0:
			return function == MCP2221_GPIO_FUNC_GPIO ||
			       function == MCP2221_GPIO_FUNC_DEDICATED ||
			       function == MCP2221_GPIO_FUNC_ALT_0;
		case MCP2221_GPIO_GP1:
			return function == MCP2221_GPIO_FUNC_GPIO ||
			       function == MCP2221_GPIO_FUNC_DEDICATED ||
			       function == MCP2221_GPIO_FUNC_ALT_0 ||
			       function == MCP2221_GPIO_FUNC_ALT_1 ||
			       function == MCP2221_GPIO_FUNC_ALT_2;
		case MCP2221_GPIO_GP2:
		case MCP2221_GPIO_GP3:
			return function == MCP2221_GPIO_FUNC_GPIO ||
			       function == MCP2221_GPIO_FUNC_DEDICATED ||
			       function == MCP2221_GPIO_FUNC_ALT_0 ||
			       function == MCP2221_GPIO_FUNC_ALT_1;
		default:
			return 0;
	}
}

static int is_valid_vrm(int value) {
	return value == MCP2221_CONFIG_KEEP ||
	       value == MCP2221_ADC_VRM_OFF ||
	       value == MCP2221_ADC_VRM_1024 ||
	       value == MCP2221_ADC_VRM_2048 ||
	       value == MCP2221_ADC_VRM_4096;
}

static int is_valid_clock_duty(int value) {
	return value == MCP2221_CONFIG_KEEP ||
	       value == MCP2221_CLK_DUTY_0 ||
	       value == MCP2221_CLK_DUTY_25 ||
	       value == MCP2221_CLK_DUTY_50 ||
	       value == MCP2221_CLK_DUTY_75;
}

static int is_valid_clock_div(int value) {
	return value == MCP2221_CONFIG_KEEP ||
	       (value >= MCP2221_CLK_DIV_1 && value <= MCP2221_CLK_DIV_7);
}

static int validate_sram_config(const mcp2221_sram_config_t *cfg) {
	for (int i = 0; i < 4; i++) {
		if (!is_keep_or_bool(cfg->gp[i].value) ||
		    !is_keep_or_bool(cfg->gp[i].direction) ||
		    !is_valid_gpio_function(i, cfg->gp[i].function))
			return 0;
	}

	if (!is_keep_or_bool(cfg->int_cfg.pos_edge) ||
	    !is_keep_or_bool(cfg->int_cfg.neg_edge) ||
	    !is_keep_or_bool(cfg->int_cfg.clear_flag))
		return 0;

	if (!is_keep_or_bool(cfg->adc_cfg.ref_src) ||
	    !is_valid_vrm(cfg->adc_cfg.vrm))
		return 0;

	if (!is_keep_or_bool(cfg->dac_ref.ref_src) ||
	    !is_valid_vrm(cfg->dac_ref.vrm))
		return 0;

	if (cfg->dac_val.value != MCP2221_CONFIG_KEEP &&
	    (cfg->dac_val.value < 0 || cfg->dac_val.value > 31))
		return 0;

	if (!is_valid_clock_duty(cfg->clk_cfg.duty) ||
	    !is_valid_clock_div(cfg->clk_cfg.div))
		return 0;

	return 1;
}

static uint8_t build_gpio_byte(uint8_t old, const mcp2221_sram_gp_config_t *c) {
	uint8_t v = old;

	// Function: bits 2..0 (MCP2221_GPIO_FUNC_*)
	if (c->function != MCP2221_CONFIG_KEEP)
		v = (v & ~0x07) | c->function;

	// Direction: bit 3
	if (c->direction != MCP2221_CONFIG_KEEP) {
		if (c->direction)
			v |= MCP2221_GPIO_DIR_IN;
		else
			v &= ~MCP2221_GPIO_DIR_IN;
	}

	// Value: bit 4
	if (c->value != MCP2221_CONFIG_KEEP) {
		if (c->value)
			v |= MCP2221_GPIO_OUT_VAL_1;
		else
			v &= ~MCP2221_GPIO_OUT_VAL_1;
	}

	return v;
}

mcp2221_error_code_t mcp2221_sram_config(mcp2221_t *dev, const mcp2221_sram_config_t *cfg) {
	if (!dev || !cfg)
		return MCP2221_ERR_INVALID;
	if (!validate_sram_config(cfg))
		return MCP2221_ERR_INVALID;

	// Ensure cached GP bytes are available (Python keeps a live cache because GPIO_write does not modify SRAM).
	(void)mcp2221_internal_ensure_gpio_status(dev);

	uint8_t getcmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t resp[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_safe(dev, &getcmd, 1, resp);
	if (err)
		return err;

	// Current GPIO bytes:
	// Prefer cached values (include GPIO_write output changes). If cache isn't valid, fall back to GET_SRAM response.
	uint8_t gp_cur[4];
	if (mcp2221_internal_gpio_status_get(dev, gp_cur) != MCP2221_ERR_OK) {
		gp_cur[0] = resp[22];
		gp_cur[1] = resp[23];
		gp_cur[2] = resp[24];
		gp_cur[3] = resp[25];
		mcp2221_internal_gpio_status_set(dev, gp_cur);
	}

	// DAC/ADC reference and DAC value (packed in GET_SRAM response bytes 6 and 7)
	uint8_t dac_ref = (resp[6] >> 5) & 0x07;
	uint8_t dac_value = resp[6] & 0x1F;
	uint8_t adc_ref = (resp[7] >> 2) & 0x07;

	// Determine if GPIO configuration is requested (Python: new_gpconf != None)
	int gp_requested = 0;
	for (int i = 0; i < 4; i++) {
		if (cfg->gp[i].value != MCP2221_CONFIG_KEEP || cfg->gp[i].direction != MCP2221_CONFIG_KEEP ||
			cfg->gp[i].function != MCP2221_CONFIG_KEEP) {
			gp_requested = 1;
			break;
		}
	}

	// Apply desired GPIO changes on top of current state
	uint8_t gp_new[4];
	for (int i = 0; i < 4; i++)
		gp_new[i] = build_gpio_byte(gp_cur[i], &cfg->gp[i]);

	// Apply desired ADC/DAC ref changes (3-bit values as used by EasyMCP2221)
	if (cfg->adc_cfg.ref_src != MCP2221_CONFIG_KEEP) {
		if (cfg->adc_cfg.ref_src)
			adc_ref |= MCP2221_ADC_REF_VRM;
		else
			adc_ref &= ~MCP2221_ADC_REF_VRM;
	}
	if (cfg->adc_cfg.vrm != MCP2221_CONFIG_KEEP) {
		adc_ref = (adc_ref & ~(0x03u << 1)) | cfg->adc_cfg.vrm;
	}

	if (cfg->dac_ref.ref_src != MCP2221_CONFIG_KEEP) {
		if (cfg->dac_ref.ref_src)
			dac_ref |= MCP2221_DAC_REF_VRM;
		else
			dac_ref &= ~MCP2221_DAC_REF_VRM;
	}
	if (cfg->dac_ref.vrm != MCP2221_CONFIG_KEEP) {
		dac_ref = (dac_ref & ~(0x03u << 1)) | cfg->dac_ref.vrm;
	}

	if (cfg->dac_val.value != MCP2221_CONFIG_KEEP)
		dac_value = (uint8_t)cfg->dac_val.value;

	/* GET SRAM byte 7 and SET SRAM byte 6 use different bit layouts.
	 * Start with all SET modification bits clear so KEEP fields remain unchanged.
	 */
	uint8_t int_conf = 0;
	int int_requested = (cfg->int_cfg.pos_edge != MCP2221_CONFIG_KEEP || cfg->int_cfg.neg_edge != MCP2221_CONFIG_KEEP ||
						 cfg->int_cfg.clear_flag != MCP2221_CONFIG_KEEP);
	if (int_requested) {
		if (cfg->int_cfg.pos_edge != MCP2221_CONFIG_KEEP) {
			if (cfg->int_cfg.pos_edge)
				int_conf = (int_conf & ~MCP2221_INT_POS_EDGE_DISABLE) | MCP2221_INT_POS_EDGE_ENABLE;
			else
				int_conf = (int_conf & ~MCP2221_INT_POS_EDGE_ENABLE) | MCP2221_INT_POS_EDGE_DISABLE;
		}
		if (cfg->int_cfg.neg_edge != MCP2221_CONFIG_KEEP) {
			if (cfg->int_cfg.neg_edge)
				int_conf = (int_conf & ~MCP2221_INT_NEG_EDGE_DISABLE) | MCP2221_INT_NEG_EDGE_ENABLE;
			else
				int_conf = (int_conf & ~MCP2221_INT_NEG_EDGE_ENABLE) | MCP2221_INT_NEG_EDGE_DISABLE;
		}
		if (cfg->int_cfg.clear_flag != MCP2221_CONFIG_KEEP) {
			if (cfg->int_cfg.clear_flag)
				int_conf |= MCP2221_INT_FLAG_CLEAR;
			else
				int_conf &= ~MCP2221_INT_FLAG_CLEAR;
		}
	}

	// Clock output config: read current from GET_SRAM response byte 5
	uint8_t clk_output = resp[5] & 0x7F;
	int clk_requested = (cfg->clk_cfg.duty != MCP2221_CONFIG_KEEP || cfg->clk_cfg.div != MCP2221_CONFIG_KEEP);
	if (cfg->clk_cfg.div != MCP2221_CONFIG_KEEP)
		clk_output = (clk_output & ~0x07) | cfg->clk_cfg.div;
	if (cfg->clk_cfg.duty != MCP2221_CONFIG_KEEP)
		clk_output = (clk_output & ~(0x03u << 3)) | cfg->clk_cfg.duty;

	// VRM workaround (EasyMCP2221.SRAM_config)
	int vrm_in_use = ((dac_ref & MCP2221_DAC_REF_VRM) != 0) || ((adc_ref & MCP2221_ADC_REF_VRM) != 0);

	uint8_t cmd[12] = {0};
	cmd[0] = MCP2221_CMD_SET_SRAM_SETTINGS;
	cmd[1] = 0;

	// Clock output
	cmd[2] = clk_requested ? (MCP2221_ALTER_CLK_OUTPUT | (clk_output & 0x7F)) : MCP2221_PRESERVE_CLK_OUTPUT;

	// EasyMCP2221 v1.8.4 always sends DAC/ADC refs + DAC value with ALTER flags (even when not explicitly requested).
	// This preserves VRM state and avoids overwriting output state in subtle edge cases.
	cmd[3] = MCP2221_ALTER_DAC_REF | (dac_ref & 0x7F);
	cmd[4] = MCP2221_ALTER_DAC_VALUE | (dac_value & 0x1F);
	cmd[5] = MCP2221_ALTER_ADC_REF | (adc_ref & 0x7F);

	// Interrupt config
	cmd[6] = int_requested ? (MCP2221_ALTER_INT_CONF | (int_conf & 0x7F)) : MCP2221_PRESERVE_INT_CONF;

	// GPIO config
	cmd[7] = gp_requested ? MCP2221_ALTER_GPIO_CONF : MCP2221_PRESERVE_GPIO_CONF;
	// Python always includes GP0..GP3 bytes in the command; they are only applied if MCP2221_ALTER_GPIO_CONF is set.
	cmd[8] = gp_new[0];
	cmd[9] = gp_new[1];
	cmd[10] = gp_new[2];
	cmd[11] = gp_new[3];

	uint8_t resp2[MCP2221_PACKET_SIZE];

	if (gp_requested && vrm_in_use) {
		// Workaround: when MCP2221_ALTER_GPIO_CONF is used, VRM may reset to VDD unless we explicitly reclaim it.
		uint8_t cmd_off[12];
		memcpy(cmd_off, cmd, sizeof(cmd_off));
		cmd_off[3] = MCP2221_ALTER_DAC_REF | (MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_OFF);
		cmd_off[4] = MCP2221_ALTER_DAC_VALUE | (dac_value & 0x1F);
		cmd_off[5] = MCP2221_ALTER_ADC_REF | (MCP2221_ADC_REF_VRM | MCP2221_ADC_VRM_OFF);

		err = mcp2221_send_cmd(dev, cmd_off, sizeof(cmd_off), resp2);
		if (err)
			return err;

		// Reclaim desired VRM settings (only refs + dac_value)
		uint8_t cmd_reclaim[12] = {0};
		cmd_reclaim[0] = MCP2221_CMD_SET_SRAM_SETTINGS;
		cmd_reclaim[3] = MCP2221_ALTER_DAC_REF | (dac_ref & 0x7F);
		cmd_reclaim[4] = MCP2221_ALTER_DAC_VALUE | (dac_value & 0x1F);
		cmd_reclaim[5] = MCP2221_ALTER_ADC_REF | (adc_ref & 0x7F);

		err = mcp2221_send_cmd(dev, cmd_reclaim, sizeof(cmd_reclaim), resp2);
		if (err == MCP2221_ERR_OK && gp_requested)
			mcp2221_internal_gpio_status_set(dev, gp_new);
		return err;
	}

	err = mcp2221_send_cmd(dev, cmd, sizeof(cmd), resp2);
	if (err == MCP2221_ERR_OK && gp_requested)
		mcp2221_internal_gpio_status_set(dev, gp_new);
	return err;
}
