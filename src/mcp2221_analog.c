#include "mcp2221_analog.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "constants.h"

/* Simple helper: SET_SRAM_SETTINGS
 *
 * cmd[0] = CMD_SET_SRAM_SETTINGS
 * cmd[1] = don't care
 * cmd[2] = clk_output or PRESERVE_CLK_OUTPUT
 * cmd[3] = dac_ref
 * cmd[4] = dac_value or PRESERVE_DAC_VALUE
 * cmd[5] = adc_ref
 * cmd[6] = int_conf or PRESERVE_INT_CONF
 * cmd[7] = new_gpconf or PRESERVE_GPIO_CONF
 * cmd[8..11] = gp0..gp3
 *
 * In order to update a single value but preserve all others.
 */

static int sram_update_simple(MCP2221 *dev, int clk_output, /* -1 = keep, else use value */
							  int dac_ref, int dac_value, int adc_ref, int int_conf) {
	uint8_t cmd[12] = {0};

	cmd[0] = CMD_SET_SRAM_SETTINGS;
	cmd[1] = 0;

	// Clock output
	if (clk_output >= 0)
		cmd[2] = ALTER_CLK_OUTPUT | (uint8_t)clk_output;
	else
		cmd[2] = PRESERVE_CLK_OUTPUT;

	// DAC reference
	if (dac_ref >= 0)
		cmd[3] = ALTER_DAC_REF | (uint8_t)dac_ref;
	else
		cmd[3] = 0; /* "not altered" */

	// DAC value
	if (dac_value >= 0)
		cmd[4] = ALTER_DAC_VALUE | (uint8_t)(dac_value & 0x1F);
	else
		cmd[4] = PRESERVE_DAC_VALUE;

	// ADC reference
	if (adc_ref >= 0)
		cmd[5] = ALTER_ADC_REF | (uint8_t)adc_ref;
	else
		cmd[5] = 0; /* "not altered" */

	// Interrupt config
	if (int_conf >= 0)
		cmd[6] = ALTER_INT_CONF | (uint8_t)int_conf;
	else
		cmd[6] = PRESERVE_INT_CONF;

	// keep GPIO config
	cmd[7] = PRESERVE_GPIO_CONF;
	cmd[8] = 0;
	cmd[9] = 0;
	cmd[10] = 0;
	cmd[11] = 0;

	uint8_t resp[PACKET_SIZE];
	mcp_err_t err = mcp2221_send_cmd(dev, cmd, sizeof(cmd), resp);
	if (err)
		return err;

	if (resp[RESPONSE_STATUS_BYTE] != RESPONSE_RESULT_OK)
		return MCP_ERR_I2C; /* generic I2C Error */

	return MCP_ERR_OK;
}

// ADC

int mcp2221_adc_config(MCP2221 *dev, const char *ref_str) {
	if (!dev || !ref_str)
		return MCP_ERR_INVALID;

	int ref_bit = -1;
	int vrm_bits = -1;

	// Like ADC_config() in MCP2221.py
	if (strcasecmp(ref_str, "OFF") == 0) {
		ref_bit = ADC_REF_VRM;
		vrm_bits = ADC_VRM_OFF;
	} else if (strcasecmp(ref_str, "1.024V") == 0) {
		ref_bit = ADC_REF_VRM;
		vrm_bits = ADC_VRM_1024;
	} else if (strcasecmp(ref_str, "2.048V") == 0) {
		ref_bit = ADC_REF_VRM;
		vrm_bits = ADC_VRM_2048;
	} else if (strcasecmp(ref_str, "4.096V") == 0) {
		ref_bit = ADC_REF_VRM;
		vrm_bits = ADC_VRM_4096;
	} else if (strcasecmp(ref_str, "VDD") == 0) {
		ref_bit = ADC_REF_VDD;
		vrm_bits = ADC_VRM_OFF;
	} else {
		return MCP_ERR_INVALID; /* like ValueError in Python */

	}

	int adc_ref = ref_bit | vrm_bits;

	// Just set adc_ref

	return sram_update_simple(dev, -1, /* keep clk_output */
							  -1,	   /* keep dac_ref */
							  -1,	   /* keep dac_value */
							  adc_ref, /* set adc_ref */
							  -1);	   /* keep int_conf */
}

int mcp2221_adc_read_raw(MCP2221 *dev, uint16_t out[3]) {
	if (!dev || !out)
		return MCP_ERR_INVALID;

	uint8_t cmd = CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t buf[PACKET_SIZE];

	int err = mcp2221_send_cmd(dev, &cmd, 1, buf);
	if (err)
		return err;

	uint16_t adc1 = buf[I2C_POLL_RESP_ADC_CH0_LSB] + ((uint16_t)buf[I2C_POLL_RESP_ADC_CH0_MSB] << 8);
	uint16_t adc2 = buf[I2C_POLL_RESP_ADC_CH1_LSB] + ((uint16_t)buf[I2C_POLL_RESP_ADC_CH1_MSB] << 8);
	uint16_t adc3 = buf[I2C_POLL_RESP_ADC_CH2_LSB] + ((uint16_t)buf[I2C_POLL_RESP_ADC_CH2_MSB] << 8);

	out[0] = adc1;
	out[1] = adc2;
	out[2] = adc3;

	return MCP_ERR_OK;
}

// DAC

static int parse_dac_ref(const char *ref_str, int *out_ref_bits) {
	if (!ref_str || !out_ref_bits)
		return MCP_ERR_INVALID;
	int ref_bit = -1;
	int vrm_bits = -1;
	if (strcasecmp(ref_str, "OFF") == 0) {
		ref_bit = DAC_REF_VRM;
		vrm_bits = DAC_VRM_OFF;
	} else if (strcasecmp(ref_str, "1.024V") == 0) {
		ref_bit = DAC_REF_VRM;
		vrm_bits = DAC_VRM_1024;
	} else if (strcasecmp(ref_str, "2.048V") == 0) {
		ref_bit = DAC_REF_VRM;
		vrm_bits = DAC_VRM_2048;
	} else if (strcasecmp(ref_str, "4.096V") == 0) {
		ref_bit = DAC_REF_VRM;
		vrm_bits = DAC_VRM_4096;
	} else if (strcasecmp(ref_str, "VDD") == 0) {
		ref_bit = DAC_REF_VDD;
		vrm_bits = DAC_VRM_OFF;
	} else {
		return MCP_ERR_INVALID;
	}
	*out_ref_bits = ref_bit | vrm_bits;
	return MCP_ERR_OK;
}

int mcp2221_dac_config_out(MCP2221 *dev, const char *ref_str, int out_code) {
	if (!dev)
		return MCP_ERR_INVALID;

	int desired_ref = 0;
	int err = parse_dac_ref(ref_str, &desired_ref);
	if (err != MCP_ERR_OK)
		return err;

	if (out_code >= 0 && out_code > 31)
		return MCP_ERR_INVALID;

	// Read current DAC ref/value from SRAM (as Python uses self.status)
	uint8_t cmd = CMD_GET_SRAM_SETTINGS;
	uint8_t resp[PACKET_SIZE];
	err = mcp2221_send_cmd(dev, &cmd, 1, resp);
	if (err != MCP_ERR_OK)
		return err;

	int current_ref = (resp[6] >> 5) & 0x07;
	int current_val = resp[6] & 0x1F;

	int desired_val = (out_code >= 0) ? out_code : current_val;

	// If reference changes, apply Python's two-step (turn off DAC, then apply new ref+value)
	if (current_ref != desired_ref) {
		// Step 1: turn off DAC (VRM OFF) and value = 0
		int r = sram_update_simple(dev, -1,		 /* keep clk_output */
								   DAC_REF_VRM | DAC_VRM_OFF, /* dac_ref off */
								   0,						  /* dac_value=0 */
								   -1,						  /* keep adc_ref */
								   -1);						  /* keep int_conf */
		if (r != MCP_ERR_OK)
			return r;
		// Step 2: set desired ref + desired value
		return sram_update_simple(dev, -1, desired_ref, desired_val, -1, -1);
	}

	// Same reference: just set ref/value once
	return sram_update_simple(dev, -1, desired_ref, desired_val, -1, -1);
}

int mcp2221_dac_config(MCP2221 *dev, const char *ref_str) {
	return mcp2221_dac_config_out(dev, ref_str, -1);
}

int mcp2221_dac_write_raw(MCP2221 *dev, uint8_t code) {
	if (!dev)
		return MCP_ERR_INVALID;
	if (code > 31)
		return MCP_ERR_INVALID;

	// only change dac_value
	return sram_update_simple(dev, -1, /* keep clk_output */
							  -1,	   /* keep dac_ref */
							  code,	   /* set dac_value */
							  -1,	   /* keep adc_ref */
							  -1);	   /* keep int_conf */
}

// Clock output

int mcp2221_clock_config(MCP2221 *dev, int duty_percent, const char *freq_str) {
	if (!dev || !freq_str)
		return MCP_ERR_INVALID;

	int duty_bits;
	if (duty_percent == 0)
		duty_bits = CLK_DUTY_0;
	else if (duty_percent == 25)
		duty_bits = CLK_DUTY_25;
	else if (duty_percent == 50)
		duty_bits = CLK_DUTY_50;
	else if (duty_percent == 75)
		duty_bits = CLK_DUTY_75;
	else
		return MCP_ERR_INVALID; /* like ValueError in Python */

	int div_bits;
	if (strcasecmp(freq_str, "375kHz") == 0)
		div_bits = CLK_FREQ_375kHz;
	else if (strcasecmp(freq_str, "750kHz") == 0)
		div_bits = CLK_FREQ_750kHz;
	else if (strcasecmp(freq_str, "1.5MHz") == 0)
		div_bits = CLK_FREQ_1_5MHz;
	else if (strcasecmp(freq_str, "3MHz") == 0)
		div_bits = CLK_FREQ_3MHz;
	else if (strcasecmp(freq_str, "6MHz") == 0)
		div_bits = CLK_FREQ_6MHz;
	else if (strcasecmp(freq_str, "12MHz") == 0)
		div_bits = CLK_FREQ_12MHz;
	else if (strcasecmp(freq_str, "24MHz") == 0)
		div_bits = CLK_FREQ_24MHz;
	else
		return MCP_ERR_INVALID;

	int clk_output = duty_bits | div_bits;

	return sram_update_simple(dev, clk_output, /* set clk_output */
							  -1,			   /* keep dac_ref */
							  -1,			   /* keep dac_value */
							  -1,			   /* keep adc_ref */
							  -1);			   /* keep int_conf */
}

// Interrupt On Change

int mcp2221_ioc_read(MCP2221 *dev, uint8_t *flag) {
	if (!dev || !flag)
		return MCP_ERR_INVALID;

	uint8_t cmd = CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t rbuf[PACKET_SIZE];

	int err = mcp2221_send_cmd(dev, &cmd, 1, rbuf);
	if (err)
		return err;

	*flag = rbuf[I2C_POLL_RESP_INT_FLAG];
	return MCP_ERR_OK;
}

int mcp2221_ioc_clear(MCP2221 *dev) {
	if (!dev)
		return MCP_ERR_INVALID;

	// Corresponds to SRAM_config(int_conf = INT_FLAG_CLEAR)
	return sram_update_simple(dev, -1,		   /* keep clk_output */
							  -1,			   /* keep dac_ref */
							  -1,			   /* keep dac_value */
							  -1,			   /* keep adc_ref */
							  INT_FLAG_CLEAR); /* set int_conf */
}

int mcp2221_ioc_config(MCP2221 *dev, const char *edge) {
	if (!dev || !edge)
		return MCP_ERR_INVALID;

	int conf;

	if (strcasecmp(edge, "none") == 0)
		conf = INT_POS_EDGE_DISABLE | INT_NEG_EDGE_DISABLE;
	else if (strcasecmp(edge, "rising") == 0)
		conf = INT_POS_EDGE_ENABLE | INT_NEG_EDGE_DISABLE;
	else if (strcasecmp(edge, "falling") == 0)
		conf = INT_POS_EDGE_DISABLE | INT_NEG_EDGE_ENABLE;
	else if (strcasecmp(edge, "both") == 0)
		conf = INT_POS_EDGE_ENABLE | INT_NEG_EDGE_ENABLE;
	else
		return MCP_ERR_INVALID;

	return sram_update_simple(dev, -1, /* keep clk_output */
							  -1,	   /* keep dac_ref */
							  -1,	   /* keep dac_value */
							  -1,	   /* keep adc_ref */
							  conf);   /* set int_conf */
}
