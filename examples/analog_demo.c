#include <stdio.h>
#include <stdint.h>

#include "mcp2221.h"
#include "mcp2221_analog.h"

static void print_err(const char *what, int err) {
	fprintf(stderr, "%s failed: %d\n", what, err);
}

int main(void) {
	MCP2221 *dev = mcp2221_open_simple(0x04D8, 0x00DD, 0, NULL, 100000);
	if (!dev) {
		fprintf(stderr, "Failed to open MCP2221\n");
		return 1;
	}

	// ADC: set 1.024V internal reference and read raw channels.
	int err = mcp2221_adc_config(dev, "1.024V");
	if (err != MCP_ERR_OK) {
		print_err("ADC_config", err);
		mcp2221_close(dev);
		return 1;
	}

	uint16_t adc[3] = {0};
	err = mcp2221_adc_read_raw(dev, adc);
	if (err != MCP_ERR_OK) {
		print_err("ADC_read_raw", err);
		mcp2221_close(dev);
		return 1;
	}
	printf("ADC raw: CH0(GP1)=%u CH1(GP2)=%u CH2(GP3)=%u\n", adc[0], adc[1], adc[2]);

	// DAC: configure 2.048V ref and set code to mid-scale, then update code once more.
	err = mcp2221_dac_config_out(dev, "2.048V", 16);
	if (err != MCP_ERR_OK) {
		print_err("DAC_config_out", err);
		mcp2221_close(dev);
		return 1;
	}

	err = mcp2221_dac_write_raw(dev, 8);
	if (err != MCP_ERR_OK) {
		print_err("DAC_write_raw", err);
		mcp2221_close(dev);
		return 1;
	}
	printf("DAC set to code 8 (ref 2.048V)\n");

	// Clock: enable 50%% duty at 12 MHz (matches Python clock_config usage).
	err = mcp2221_clock_config(dev, 50, "12MHz");
	if (err != MCP_ERR_OK) {
		print_err("clock_config", err);
		mcp2221_close(dev);
		return 1;
	}
	printf("Clock output set to 12MHz, 50%% duty\n");

	mcp2221_close(dev);
	return 0;
}
