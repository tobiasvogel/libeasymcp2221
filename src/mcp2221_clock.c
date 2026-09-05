#include "mcp2221_clock.h"
#include "mcp2221_constants.h"
#include "mcp2221_sram.h"

static int clock_duty_selector(unsigned duty_percent, int *selector) {
    switch (duty_percent) {
    case 0:  *selector = MCP2221_CLK_DUTY_0;  return 1;
    case 25: *selector = MCP2221_CLK_DUTY_25; return 1;
    case 50: *selector = MCP2221_CLK_DUTY_50; return 1;
    case 75: *selector = MCP2221_CLK_DUTY_75; return 1;
    default: return 0;
    }
}

static int clock_frequency_selector(uint32_t frequency_hz, int *selector) {
    switch (frequency_hz) {
    case 375000u:   *selector = MCP2221_CLK_FREQ_375kHz; return 1;
    case 750000u:   *selector = MCP2221_CLK_FREQ_750kHz; return 1;
    case 1500000u:  *selector = MCP2221_CLK_FREQ_1_5MHz; return 1;
    case 3000000u:  *selector = MCP2221_CLK_FREQ_3MHz;   return 1;
    case 6000000u:  *selector = MCP2221_CLK_FREQ_6MHz;   return 1;
    case 12000000u: *selector = MCP2221_CLK_FREQ_12MHz;  return 1;
    case 24000000u: *selector = MCP2221_CLK_FREQ_24MHz;  return 1;
    default: return 0;
    }
}

mcp2221_error_code_t mcp2221_clock_config(
    mcp2221_t *dev, unsigned duty_percent, uint32_t frequency_hz) {
    if (!dev)
        return MCP2221_ERR_INVALID;

    int duty;
    int div;
    if (!clock_duty_selector(duty_percent, &duty) ||
        !clock_frequency_selector(frequency_hz, &div))
        return MCP2221_ERR_INVALID;

    mcp2221_sram_config_t cfg;
    for (int i = 0; i < 4; i++) {
        cfg.gp[i].value = MCP2221_CONFIG_KEEP;
        cfg.gp[i].direction = MCP2221_CONFIG_KEEP;
        cfg.gp[i].function = MCP2221_CONFIG_KEEP;
    }
    cfg.int_cfg.pos_edge = MCP2221_CONFIG_KEEP;
    cfg.int_cfg.neg_edge = MCP2221_CONFIG_KEEP;
    cfg.int_cfg.clear_flag = MCP2221_CONFIG_KEEP;
    cfg.adc_cfg.vrm = MCP2221_CONFIG_KEEP;
    cfg.adc_cfg.ref_src = MCP2221_CONFIG_KEEP;
    cfg.dac_ref.vrm = MCP2221_CONFIG_KEEP;
    cfg.dac_ref.ref_src = MCP2221_CONFIG_KEEP;
    cfg.dac_val.value = MCP2221_CONFIG_KEEP;
    cfg.clk_cfg.duty = duty;
    cfg.clk_cfg.div = div;

    return mcp2221_sram_config(dev, &cfg);
}
