#include "mcp2221_internal.h"
#include "mcp2221_sram.h"

#include "mcp2221_internal.h"
#include <string.h>

#include "constants.h"
#include "exceptions.h"

// Internal helpers implemented in src/mcp2221.c (not part of the public API)

static uint8_t build_gpio_byte(uint8_t old, const MCP_SRAM_GP_Config *c) {
	uint8_t v = old;

	// Function: bits 2..0 (GPIO_FUNC_xxx)
	if (c->function != MCP_CONFIG_KEEP)
		v = (v & ~0x07) | (c->function & 0x07);

	// Direction: bit 3
	if (c->direction != MCP_CONFIG_KEEP) {
		if (c->direction)
			v |= GPIO_DIR_IN;
		else
			v &= ~GPIO_DIR_IN;
	}

	// Value: bit 4
	if (c->value != MCP_CONFIG_KEEP) {
		if (c->value)
			v |= GPIO_OUT_VAL_1;
		else
			v &= ~GPIO_OUT_VAL_1;
	}

	return v;
}

int mcp2221_sram_config(MCP2221 *dev, const MCP2221_SRAM_Config *cfg) {
	if (!dev || !cfg)
		return MCP_ERR_INVALID;

	// Ensure cached GP bytes are available (Python keeps a live cache because GPIO_write does not modify SRAM).
	(void)mcp2221_internal_ensure_gpio_status(dev);

	uint8_t getcmd = CMD_GET_SRAM_SETTINGS;
	uint8_t resp[PACKET_SIZE];

	int err = mcp2221_send_cmd(dev, &getcmd, 1, resp);
	if (err)
		return err;

	// Current GPIO bytes:
	// Prefer cached values (include GPIO_write output changes). If cache isn't valid, fall back to GET_SRAM response.
	uint8_t gp_cur[4];
	if (mcp2221_internal_gpio_status_get(dev, gp_cur) != MCP_ERR_OK) {
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
		if (cfg->gp[i].value != MCP_CONFIG_KEEP || cfg->gp[i].direction != MCP_CONFIG_KEEP ||
			cfg->gp[i].function != MCP_CONFIG_KEEP) {
			gp_requested = 1;
			break;
		}
	}

	// Apply desired GPIO changes on top of current state
	uint8_t gp_new[4];
	for (int i = 0; i < 4; i++)
		gp_new[i] = build_gpio_byte(gp_cur[i], &cfg->gp[i]);

	// Apply desired ADC/DAC ref changes (3-bit values as used by EasyMCP2221)
	if (cfg->adc_cfg.ref_src != MCP_CONFIG_KEEP) {
		if (cfg->adc_cfg.ref_src)
			adc_ref |= ADC_REF_VRM;
		else
			adc_ref &= ~ADC_REF_VRM;
	}
	if (cfg->adc_cfg.vrm != MCP_CONFIG_KEEP) {
		adc_ref = (adc_ref & ~(0b11 << 1)) | (cfg->adc_cfg.vrm & (0b11 << 1));
	}

	if (cfg->dac_ref.ref_src != MCP_CONFIG_KEEP) {
		if (cfg->dac_ref.ref_src)
			dac_ref |= DAC_REF_VRM;
		else
			dac_ref &= ~DAC_REF_VRM;
	}
	if (cfg->dac_ref.vrm != MCP_CONFIG_KEEP) {
		dac_ref = (dac_ref & ~(0b11 << 1)) | (cfg->dac_ref.vrm & (0b11 << 1));
	}

	if (cfg->dac_val.value != MCP_CONFIG_KEEP)
		dac_value = (uint8_t)cfg->dac_val.value & 0x1F;

	// Interrupt config: build from current GET_SRAM packed byte (low 5 bits)
	uint8_t int_conf = resp[7] & 0x1F;
	int int_requested = (cfg->int_cfg.pos_edge != MCP_CONFIG_KEEP || cfg->int_cfg.neg_edge != MCP_CONFIG_KEEP ||
						 cfg->int_cfg.clear_flag != MCP_CONFIG_KEEP);
	if (int_requested) {
		if (cfg->int_cfg.pos_edge != MCP_CONFIG_KEEP) {
			if (cfg->int_cfg.pos_edge)
				int_conf = (int_conf & ~INT_POS_EDGE_DISABLE) | INT_POS_EDGE_ENABLE;
			else
				int_conf = (int_conf & ~INT_POS_EDGE_ENABLE) | INT_POS_EDGE_DISABLE;
		}
		if (cfg->int_cfg.neg_edge != MCP_CONFIG_KEEP) {
			if (cfg->int_cfg.neg_edge)
				int_conf = (int_conf & ~INT_NEG_EDGE_DISABLE) | INT_NEG_EDGE_ENABLE;
			else
				int_conf = (int_conf & ~INT_NEG_EDGE_ENABLE) | INT_NEG_EDGE_DISABLE;
		}
		if (cfg->int_cfg.clear_flag != MCP_CONFIG_KEEP) {
			if (cfg->int_cfg.clear_flag)
				int_conf |= INT_FLAG_CLEAR;
			else
				int_conf &= ~INT_FLAG_CLEAR;
		}
	}

	// Clock output config: read current from GET_SRAM response byte 5
	uint8_t clk_output = resp[5] & 0x7F;
	int clk_requested = (cfg->clk_cfg.duty != MCP_CONFIG_KEEP || cfg->clk_cfg.div != MCP_CONFIG_KEEP);
	if (cfg->clk_cfg.div != MCP_CONFIG_KEEP)
		clk_output = (clk_output & ~0x07) | (cfg->clk_cfg.div & 0x07);
	if (cfg->clk_cfg.duty != MCP_CONFIG_KEEP)
		clk_output = (clk_output & ~(0b11 << 3)) | (cfg->clk_cfg.duty & (0b11 << 3));

	// VRM workaround (EasyMCP2221.SRAM_config)
	int vrm_in_use = ((dac_ref & DAC_REF_VRM) != 0) || ((adc_ref & ADC_REF_VRM) != 0);

	uint8_t cmd[12] = {0};
	cmd[0] = CMD_SET_SRAM_SETTINGS;
	cmd[1] = 0;

	// Clock output
	cmd[2] = clk_requested ? (ALTER_CLK_OUTPUT | (clk_output & 0x7F)) : PRESERVE_CLK_OUTPUT;

	// EasyMCP2221 v1.8.4 always sends DAC/ADC refs + DAC value with ALTER flags (even when not explicitly requested).
	// This preserves VRM state and avoids overwriting output state in subtle edge cases.
	cmd[3] = ALTER_DAC_REF | (dac_ref & 0x7F);
	cmd[4] = ALTER_DAC_VALUE | (dac_value & 0x1F);
	cmd[5] = ALTER_ADC_REF | (adc_ref & 0x7F);

	// Interrupt config
	cmd[6] = int_requested ? (ALTER_INT_CONF | (int_conf & 0x7F)) : PRESERVE_INT_CONF;

	// GPIO config
	cmd[7] = gp_requested ? ALTER_GPIO_CONF : PRESERVE_GPIO_CONF;
	// Python always includes GP0..GP3 bytes in the command; they are only applied if ALTER_GPIO_CONF is set.
	cmd[8] = gp_new[0];
	cmd[9] = gp_new[1];
	cmd[10] = gp_new[2];
	cmd[11] = gp_new[3];

	uint8_t resp2[PACKET_SIZE];

	if (gp_requested && vrm_in_use) {
		// Workaround: when ALTER_GPIO_CONF is used, VRM may reset to VDD unless we explicitly reclaim it.
		uint8_t cmd_off[12];
		memcpy(cmd_off, cmd, sizeof(cmd_off));
		cmd_off[3] = ALTER_DAC_REF | (DAC_REF_VRM | DAC_VRM_OFF);
		cmd_off[4] = ALTER_DAC_VALUE | (dac_value & 0x1F);
		cmd_off[5] = ALTER_ADC_REF | (ADC_REF_VRM | ADC_VRM_OFF);

		err = mcp2221_send_cmd(dev, cmd_off, sizeof(cmd_off), resp2);
		if (err)
			return err;

		// Reclaim desired VRM settings (only refs + dac_value)
		uint8_t cmd_reclaim[12] = {0};
		cmd_reclaim[0] = CMD_SET_SRAM_SETTINGS;
		cmd_reclaim[3] = ALTER_DAC_REF | (dac_ref & 0x7F);
		cmd_reclaim[4] = ALTER_DAC_VALUE | (dac_value & 0x1F);
		cmd_reclaim[5] = ALTER_ADC_REF | (adc_ref & 0x7F);

		err = mcp2221_send_cmd(dev, cmd_reclaim, sizeof(cmd_reclaim), resp2);
		if (err == MCP_ERR_OK && gp_requested)
			mcp2221_internal_gpio_status_set(dev, gp_new);
		return err;
	}

	err = mcp2221_send_cmd(dev, cmd, sizeof(cmd), resp2);
	if (err == MCP_ERR_OK && gp_requested)
		mcp2221_internal_gpio_status_set(dev, gp_new);
	return err;
}
