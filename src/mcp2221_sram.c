#include "mcp2221_sram.h"

#include <string.h>

#include "constants.h"
#include "exceptions.h"

static inline uint8_t make_alter(uint8_t val, uint8_t alter_mask) {
	return (alter_mask | (val & 0x7F));
}

static uint8_t build_gpio_byte(uint8_t old, const MCP_SRAM_GP_Config *c) {
	uint8_t v = old;

	// bits 6..5
	if (c->function != MCP_CONFIG_KEEP)
		v = (v & 0x9F) | ((c->function & 0x03) << 5);

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

	uint8_t getcmd = CMD_GET_SRAM_SETTINGS;
	uint8_t resp[PACKET_SIZE];

	// read SRAM
	int err = mcp2221_send_cmd(dev, &getcmd, 1, resp);
	if (err)
		return err;

	// SET-SRAM
	uint8_t buf[PACKET_SIZE];
	memset(buf, 0, sizeof(buf));
	buf[0] = CMD_SET_SRAM_SETTINGS;

	// GPIO GP0..GP3
	const int gp_offsets[4] = {SRAM_GP_SETTINGS_GP0, SRAM_GP_SETTINGS_GP1, SRAM_GP_SETTINGS_GP2, SRAM_GP_SETTINGS_GP3};

	for (int i = 0; i < 4; i++) {
		int off = 4 + gp_offsets[i];
		uint8_t oldv = resp[off];
		uint8_t newv = build_gpio_byte(oldv, &cfg->gp[i]);

		if (newv != oldv)
			buf[off] = ALTER_GPIO_CONF | (newv & 0x7F);
		else
			buf[off] = PRESERVE_GPIO_CONF;
	}

	// Interrupt Config
	{
		int off = 4 + SRAM_CHIP_SETTINGS_INT_ADC;
		uint8_t oldv = resp[off];
		uint8_t newv = oldv;

		// RISING edge
		if (cfg->int_cfg.pos_edge != MCP_CONFIG_KEEP) {
			if (cfg->int_cfg.pos_edge)
				newv = (newv & ~INT_POS_EDGE_DISABLE) | INT_POS_EDGE_ENABLE;
			else
				newv = (newv & ~INT_POS_EDGE_ENABLE) | INT_POS_EDGE_DISABLE;
		}

		// FALLING edge
		if (cfg->int_cfg.neg_edge != MCP_CONFIG_KEEP) {
			if (cfg->int_cfg.neg_edge)
				newv = (newv & ~INT_NEG_EDGE_DISABLE) | INT_NEG_EDGE_ENABLE;
			else
				newv = (newv & ~INT_NEG_EDGE_ENABLE) | INT_NEG_EDGE_DISABLE;
		}

		// clear flag
		if (cfg->int_cfg.clear_flag != MCP_CONFIG_KEEP) {
			if (cfg->int_cfg.clear_flag)
				newv |= INT_FLAG_CLEAR;
			else
				newv &= ~INT_FLAG_CLEAR;
		}

		if (newv != oldv)
			buf[off] = ALTER_INT_CONF | (newv & 0x7F);
		else
			buf[off] = PRESERVE_INT_CONF;
	}

	// ADC Reference
	{
		int off = 4 + SRAM_CHIP_SETTINGS_INT_ADC + 1;
		uint8_t oldv = resp[off];
		uint8_t newv = oldv;

		if (cfg->adc_cfg.ref_src != MCP_CONFIG_KEEP) {
			if (cfg->adc_cfg.ref_src)
				newv |= ADC_REF_VRM;
			else
				newv &= ~ADC_REF_VRM;
		}

		if (cfg->adc_cfg.vrm != MCP_CONFIG_KEEP) {
			newv = (newv & ~(0b11 << 1)) | (cfg->adc_cfg.vrm & (0b11 << 1));
		}

		if (newv != oldv)
			buf[off] = ALTER_ADC_REF | (newv & 0x7F);
	}

	// DAC Reference
	{
		int off = 4 + SRAM_CHIP_SETTINGS_INT_ADC + 2;
		uint8_t oldv = resp[off];
		uint8_t newv = oldv;

		if (cfg->dac_ref.ref_src != MCP_CONFIG_KEEP) {
			if (cfg->dac_ref.ref_src)
				newv |= DAC_REF_VRM;
			else
				newv &= ~DAC_REF_VRM;
		}

		if (cfg->dac_ref.vrm != MCP_CONFIG_KEEP) {
			newv = (newv & ~(0b11 << 1)) | (cfg->dac_ref.vrm & (0b11 << 1));
		}

		if (newv != oldv)
			buf[off] = ALTER_DAC_REF | (newv & 0x7F);
	}

	// DAC Value
	{
		int off = 4 + SRAM_CHIP_SETTINGS_INT_ADC + 3;
		uint8_t oldv = resp[off];
		uint8_t newv = oldv;

		if (cfg->dac_val.value != MCP_CONFIG_KEEP) {
			newv = (newv & ~0x1F) | (cfg->dac_val.value & 0x1F);
		}

		if (newv != oldv)
			buf[off] = ALTER_DAC_VALUE | (newv & 0x7F);
	}

	// Clock Output
	{
		int off = 4 + SRAM_CHIP_SETTINGS_INT_ADC + 4;
		uint8_t oldv = resp[off];
		uint8_t newv = oldv;

		if (cfg->clk_cfg.div != MCP_CONFIG_KEEP)
			newv = (newv & ~0x07) | (cfg->clk_cfg.div & 0x07);

		if (cfg->clk_cfg.duty != MCP_CONFIG_KEEP)
			newv = (newv & ~(0b11 << 3)) | (cfg->clk_cfg.duty & (0b11 << 3));

		if (newv != oldv)
			buf[off] = ALTER_CLK_OUTPUT | (newv & 0x7F);
	}

	// Send packet
	uint8_t resp2[PACKET_SIZE];
	return mcp2221_send_cmd(dev, buf, PACKET_SIZE, resp2);
}
