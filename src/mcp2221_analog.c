#include "mcp2221_analog.h"

#include "mcp2221_internal_constants.h"
#include "mcp2221_internal.h"
#include "mcp2221_internal_analog.h"
#include "mcp2221_internal_string.h"

/*
 * Narrow SET_SRAM_SETTINGS path for analog, clock and IOC operations.
 *
 * This intentionally does not use mcp2221_sram_config(). The full SRAM
 * configuration path mirrors EasyMCP2221's state handling and contains
 * GPIO/VRM workarounds for MCP2221 hardware quirks.
 *
 * These helpers preserve GPIO configuration and alter only the explicitly
 * requested SRAM fields, matching the original EasyMCP2221 command semantics
 * and avoiding unnecessary GPIO/VRM interaction.
 *
 * Do not consolidate these paths without re-validating the MCP2221 VRM, GPIO
 * and DAC-reference quirks against the original EasyMCP2221 implementation
 * and hardware.
 *
 * cmd[0] = MCP2221_CMD_SET_SRAM_SETTINGS
 * cmd[1] = don't care
 * cmd[2] = clk_output or MCP2221_PRESERVE_CLK_OUTPUT
 * cmd[3] = dac_ref
 * cmd[4] = dac_value or MCP2221_PRESERVE_DAC_VALUE
 * cmd[5] = adc_ref
 * cmd[6] = int_conf or MCP2221_PRESERVE_INT_CONF
 * cmd[7] = MCP2221_PRESERVE_GPIO_CONF
 * cmd[8..11] = 0
 */

static mcp2221_error_code_t set_sram_fields_preserve_gpio(mcp2221_t *dev, int clk_output, /* -1 = keep, else use value */
							  int dac_ref, int dac_value, int adc_ref, int int_conf) {
	uint8_t cmd[12] = {0};

	cmd[0] = MCP2221_CMD_SET_SRAM_SETTINGS;
	cmd[1] = 0;

	// Clock output
	if (clk_output >= 0)
		cmd[2] = MCP2221_ALTER_CLK_OUTPUT | (uint8_t)clk_output;
	else
		cmd[2] = MCP2221_PRESERVE_CLK_OUTPUT;

	// DAC reference
	if (dac_ref >= 0)
		cmd[3] = MCP2221_ALTER_DAC_REF | (uint8_t)dac_ref;
	else
		cmd[3] = 0; /* "not altered" */

	// DAC value
	if (dac_value >= 0)
		cmd[4] = MCP2221_ALTER_DAC_VALUE | (uint8_t)(dac_value & 0x1F);
	else
		cmd[4] = MCP2221_PRESERVE_DAC_VALUE;

	// ADC reference
	if (adc_ref >= 0)
		cmd[5] = MCP2221_ALTER_ADC_REF | (uint8_t)adc_ref;
	else
		cmd[5] = 0; /* "not altered" */

	// Interrupt config
	if (int_conf >= 0)
		cmd[6] = MCP2221_ALTER_INT_CONF | (uint8_t)int_conf;
	else
		cmd[6] = MCP2221_PRESERVE_INT_CONF;

	// keep GPIO config
	cmd[7] = MCP2221_PRESERVE_GPIO_CONF;
	cmd[8] = 0;
	cmd[9] = 0;
	cmd[10] = 0;
	cmd[11] = 0;

	uint8_t resp[MCP2221_PACKET_SIZE];
	mcp2221_error_code_t err = mcp2221_send_cmd(dev, cmd, sizeof(cmd), resp);
	if (err)
		return err;

	return MCP2221_ERR_OK;
}

// Shared analog configuration

mcp2221_error_code_t mcp2221_analog_set_vdd(
	mcp2221_t *dev,
	double volts) {
	return mcp2221_internal_analog_set_vdd(dev, volts);
}

mcp2221_error_code_t mcp2221_analog_get_vdd(
	const mcp2221_t *dev,
	double *volts) {
	return mcp2221_internal_analog_get_vdd(dev, volts);
}

// ADC

mcp2221_error_code_t mcp2221_adc_config(
	mcp2221_t *dev,
	const char *ref_str) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	mcp2221_analog_voltage_reference_t reference;
	mcp2221_error_code_t err =
		mcp2221_internal_analog_parse_voltage_reference(
			ref_str,
			&reference);
	if (err != MCP2221_ERR_OK)
		return err;

	int adc_ref;
	err = mcp2221_internal_analog_adc_reference_to_bits(
		reference,
		&adc_ref);
	if (err != MCP2221_ERR_OK)
		return err;

	return set_sram_fields_preserve_gpio(
		dev,
		-1,      /* keep clk_output */
		-1,      /* keep dac_ref */
		-1,      /* keep dac_value */
		adc_ref, /* set adc_ref */
		-1);     /* keep int_conf */
}

mcp2221_error_code_t mcp2221_adc_read_raw(mcp2221_t *dev, uint16_t out[3]) {
	if (!dev || !out)
		return MCP2221_ERR_INVALID;

	uint8_t cmd = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t buf[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_safe(dev, &cmd, 1, buf);
	if (err)
		return err;

	uint16_t adc1 = buf[MCP2221_I2C_POLL_RESP_ADC_CH0_LSB] + ((uint16_t)buf[MCP2221_I2C_POLL_RESP_ADC_CH0_MSB] << 8);
	uint16_t adc2 = buf[MCP2221_I2C_POLL_RESP_ADC_CH1_LSB] + ((uint16_t)buf[MCP2221_I2C_POLL_RESP_ADC_CH1_MSB] << 8);
	uint16_t adc3 = buf[MCP2221_I2C_POLL_RESP_ADC_CH2_LSB] + ((uint16_t)buf[MCP2221_I2C_POLL_RESP_ADC_CH2_MSB] << 8);

	if (adc1 > MCP2221_ADC_RAW_MAX ||
	    adc2 > MCP2221_ADC_RAW_MAX ||
	    adc3 > MCP2221_ADC_RAW_MAX)
		return MCP2221_ERR_PROTOCOL;

	out[0] = adc1;
	out[1] = adc2;
	out[2] = adc3;

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_adc_read_normalized(
	mcp2221_t *dev,
	double out[3]) {
	if (!dev || !out)
		return MCP2221_ERR_INVALID;

	uint16_t raw[3];
	mcp2221_error_code_t err = mcp2221_adc_read_raw(dev, raw);
	if (err != MCP2221_ERR_OK)
		return err;

	for (int i = 0; i < 3; i++) {
		err = mcp2221_internal_analog_adc_raw_to_normalized(
			raw[i],
			&out[i]);
		if (err != MCP2221_ERR_OK)
			return err;
	}

	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_adc_read_volts(
	mcp2221_t *dev,
	double out[3]) {
	if (!dev || !out)
		return MCP2221_ERR_INVALID;

	/*
	 * Read the currently configured ADC reference from SRAM.
	 * Byte 7 contains the interrupt and ADC reference settings.
	 */
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t resp[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err =
		mcp2221_internal_send_cmd_retry_safe(dev, &cmd, 1, resp);
	if (err != MCP2221_ERR_OK)
		return err;

	mcp2221_analog_voltage_reference_t reference;
	err = mcp2221_internal_analog_adc_reference_from_bits(
		resp[MCP2221_SRAM_RESPONSE_INT_ADC],
		&reference);
	if (err != MCP2221_ERR_OK)
		return err;

	double reference_voltage;
	err = mcp2221_internal_analog_get_reference_voltage(
		dev,
		reference,
		&reference_voltage);
	if (err != MCP2221_ERR_OK)
		return err;

	uint16_t raw[3];
	err = mcp2221_adc_read_raw(dev, raw);
	if (err != MCP2221_ERR_OK)
		return err;

	for (int i = 0; i < 3; i++) {
		err = mcp2221_internal_analog_adc_raw_to_volts(
			raw[i],
			reference_voltage,
			&out[i]);
		if (err != MCP2221_ERR_OK)
			return err;
	}

	return MCP2221_ERR_OK;
}

// DAC

mcp2221_error_code_t mcp2221_dac_config_out(
	mcp2221_t *dev,
	const char *ref_str,
	int out_code) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	mcp2221_analog_voltage_reference_t reference;
	mcp2221_error_code_t err =
		mcp2221_internal_analog_parse_voltage_reference(
			ref_str,
			&reference);
	if (err != MCP2221_ERR_OK)
		return err;

	int desired_ref;
	err = mcp2221_internal_analog_dac_reference_to_bits(
		reference,
		&desired_ref);
	if (err != MCP2221_ERR_OK)
		return err;

	if (out_code >= 0 && out_code > 31)
		return MCP2221_ERR_INVALID;

	// Read current DAC ref/value from SRAM (as Python uses self.status)
	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t resp[MCP2221_PACKET_SIZE];
	err = mcp2221_internal_send_cmd_retry_safe(dev, &cmd, 1, resp);
	if (err != MCP2221_ERR_OK)
		return err;

	int current_ref = (resp[MCP2221_SRAM_RESPONSE_DAC] >> 5) & 0x07;
	int current_val = resp[MCP2221_SRAM_RESPONSE_DAC] & 0x1F;

	int desired_val = (out_code >= 0) ? out_code : current_val;

	/*
	 * Preserve EasyMCP2221's two-step DAC-reference change sequence.
	 * Turning the DAC off before applying a new reference/value avoids a
	 * known MCP2221 ADC/DAC reference-transition quirk.
	 */
	if (current_ref != desired_ref) {
		// Step 1: turn off DAC (VRM OFF) and value = 0
		mcp2221_error_code_t r = set_sram_fields_preserve_gpio(dev, -1,		 /* keep clk_output */
								   MCP2221_DAC_REF_VRM | MCP2221_DAC_VRM_OFF, /* dac_ref off */
								   0,						  /* dac_value=0 */
								   -1,						  /* keep adc_ref */
								   -1);						  /* keep int_conf */
		if (r != MCP2221_ERR_OK)
			return r;
		// Step 2: set desired ref + desired value
		return set_sram_fields_preserve_gpio(dev, -1, desired_ref, desired_val, -1, -1);
	}

	// Same reference: just set ref/value once
	return set_sram_fields_preserve_gpio(dev, -1, desired_ref, desired_val, -1, -1);
}

mcp2221_error_code_t mcp2221_dac_config(mcp2221_t *dev, const char *ref_str) {
	return mcp2221_dac_config_out(dev, ref_str, -1);
}

mcp2221_error_code_t mcp2221_dac_write_raw(mcp2221_t *dev, uint8_t code) {
	if (!dev)
		return MCP2221_ERR_INVALID;
	if (code > 31)
		return MCP2221_ERR_INVALID;

	// only change dac_value
	return set_sram_fields_preserve_gpio(dev, -1, /* keep clk_output */
							  -1,	   /* keep dac_ref */
							  code,	   /* set dac_value */
							  -1,	   /* keep adc_ref */
							  -1);	   /* keep int_conf */
}

mcp2221_error_code_t mcp2221_dac_write_normalized(
	mcp2221_t *dev,
	double normalized) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	uint8_t raw;
	mcp2221_error_code_t err =
		mcp2221_internal_analog_dac_normalized_to_raw(
			normalized,
			&raw);
	if (err != MCP2221_ERR_OK)
		return err;

	return mcp2221_dac_write_raw(dev, raw);
}

mcp2221_error_code_t mcp2221_dac_write_volts(
	mcp2221_t *dev,
	double volts) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	uint8_t cmd = MCP2221_CMD_GET_SRAM_SETTINGS;
	uint8_t resp[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err =
		mcp2221_internal_send_cmd_retry_safe(dev, &cmd, 1, resp);
	if (err != MCP2221_ERR_OK)
		return err;

	/*
	 * The DAC reference occupies bits 5..7 of the SRAM DAC byte.
	 * Shift it down to the register-bit representation used by the
	 * internal reference decoder.
	 */
	uint8_t reference_bits =
		(uint8_t)((resp[MCP2221_SRAM_RESPONSE_DAC] >> 5) & 0x07);

	mcp2221_analog_voltage_reference_t reference;
	err = mcp2221_internal_analog_dac_reference_from_bits(
		reference_bits,
		&reference);
	if (err != MCP2221_ERR_OK)
		return err;

	double reference_voltage;
	err = mcp2221_internal_analog_get_reference_voltage(
		dev,
		reference,
		&reference_voltage);
	if (err != MCP2221_ERR_OK)
		return err;

	uint8_t raw;
	err = mcp2221_internal_analog_dac_volts_to_raw(
		volts,
		reference_voltage,
		&raw);
	if (err != MCP2221_ERR_OK)
		return err;

	return mcp2221_dac_write_raw(dev, raw);
}

// Clock output

mcp2221_error_code_t mcp2221_clock_config(mcp2221_t *dev, int duty_percent, const char *freq_str) {
	if (!dev || !freq_str)
		return MCP2221_ERR_INVALID;

	int duty_bits;
	if (duty_percent == 0)
		duty_bits = MCP2221_CLK_DUTY_0;
	else if (duty_percent == 25)
		duty_bits = MCP2221_CLK_DUTY_25;
	else if (duty_percent == 50)
		duty_bits = MCP2221_CLK_DUTY_50;
	else if (duty_percent == 75)
		duty_bits = MCP2221_CLK_DUTY_75;
	else
		return MCP2221_ERR_INVALID; /* like ValueError in Python */

	int div_bits;
	if (mcp2221_internal_ascii_case_equal(freq_str, "375kHz"))
		div_bits = MCP2221_CLK_FREQ_375kHz;
	else if (mcp2221_internal_ascii_case_equal(freq_str, "750kHz"))
		div_bits = MCP2221_CLK_FREQ_750kHz;
	else if (mcp2221_internal_ascii_case_equal(freq_str, "1.5MHz"))
		div_bits = MCP2221_CLK_FREQ_1_5MHz;
	else if (mcp2221_internal_ascii_case_equal(freq_str, "3MHz"))
		div_bits = MCP2221_CLK_FREQ_3MHz;
	else if (mcp2221_internal_ascii_case_equal(freq_str, "6MHz"))
		div_bits = MCP2221_CLK_FREQ_6MHz;
	else if (mcp2221_internal_ascii_case_equal(freq_str, "12MHz"))
		div_bits = MCP2221_CLK_FREQ_12MHz;
	else if (mcp2221_internal_ascii_case_equal(freq_str, "24MHz"))
		div_bits = MCP2221_CLK_FREQ_24MHz;
	else
		return MCP2221_ERR_INVALID;

	int clk_output = duty_bits | div_bits;

	return set_sram_fields_preserve_gpio(dev, clk_output, /* set clk_output */
							  -1,			   /* keep dac_ref */
							  -1,			   /* keep dac_value */
							  -1,			   /* keep adc_ref */
							  -1);			   /* keep int_conf */
}

// Interrupt On Change

mcp2221_error_code_t mcp2221_ioc_read(mcp2221_t *dev, uint8_t *flag) {
	if (!dev || !flag)
		return MCP2221_ERR_INVALID;

	uint8_t cmd = MCP2221_CMD_POLL_STATUS_SET_PARAMETERS;
	uint8_t rbuf[MCP2221_PACKET_SIZE];

	mcp2221_error_code_t err = mcp2221_internal_send_cmd_retry_safe(dev, &cmd, 1, rbuf);
	if (err)
		return err;

	uint8_t state = rbuf[MCP2221_I2C_POLL_RESP_INT_FLAG];
	if (state != MCP2221_INT_EDGE_STATE_INACTIVE &&
	    state != MCP2221_INT_EDGE_STATE_ACTIVE)
		return MCP2221_ERR_PROTOCOL;

	*flag = state;
	return MCP2221_ERR_OK;
}

mcp2221_error_code_t mcp2221_ioc_clear(mcp2221_t *dev) {
	if (!dev)
		return MCP2221_ERR_INVALID;

	// Corresponds to SRAM_config(int_conf = MCP2221_INT_FLAG_CLEAR)
	return set_sram_fields_preserve_gpio(dev, -1,		   /* keep clk_output */
							  -1,			   /* keep dac_ref */
							  -1,			   /* keep dac_value */
							  -1,			   /* keep adc_ref */
							  MCP2221_INT_FLAG_CLEAR); /* set int_conf */
}

mcp2221_error_code_t mcp2221_ioc_config(mcp2221_t *dev, const char *edge) {
	if (!dev || !edge)
		return MCP2221_ERR_INVALID;

	int conf;

	if (mcp2221_internal_ascii_case_equal(edge, "none"))
		conf = MCP2221_INT_POS_EDGE_DISABLE | MCP2221_INT_NEG_EDGE_DISABLE;
	else if (mcp2221_internal_ascii_case_equal(edge, "rising"))
		conf = MCP2221_INT_POS_EDGE_ENABLE | MCP2221_INT_NEG_EDGE_DISABLE;
	else if (mcp2221_internal_ascii_case_equal(edge, "falling"))
		conf = MCP2221_INT_POS_EDGE_DISABLE | MCP2221_INT_NEG_EDGE_ENABLE;
	else if (mcp2221_internal_ascii_case_equal(edge, "both"))
		conf = MCP2221_INT_POS_EDGE_ENABLE | MCP2221_INT_NEG_EDGE_ENABLE;
	else
		return MCP2221_ERR_INVALID;

	return set_sram_fields_preserve_gpio(dev, -1, /* keep clk_output */
							  -1,	   /* keep dac_ref */
							  -1,	   /* keep dac_value */
							  -1,	   /* keep adc_ref */
							  conf);   /* set int_conf */
}
