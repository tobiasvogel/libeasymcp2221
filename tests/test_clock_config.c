#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mcp2221_clock.h"
#include "mcp2221_constants.h"
#include "mcp2221_sram.h"

static int stub_calls;
static mcp2221_sram_config_t stub_cfg;
static mcp2221_error_code_t stub_result;

mcp2221_error_code_t test_sram_config(
    mcp2221_t *dev, const mcp2221_sram_config_t *cfg);

#define mcp2221_sram_config test_sram_config
#include "../src/mcp2221_clock.c"
#undef mcp2221_sram_config

mcp2221_error_code_t test_sram_config(
    mcp2221_t *dev, const mcp2221_sram_config_t *cfg) {
    assert(dev != NULL);
    assert(cfg != NULL);
    stub_calls++;
    stub_cfg = *cfg;
    return stub_result;
}

static void reset_stub(void) {
    stub_calls = 0;
    memset(&stub_cfg, 0, sizeof(stub_cfg));
    stub_result = MCP2221_ERR_OK;
}

static void assert_non_clock_fields_keep(void) {
    for (int i = 0; i < 4; i++) {
        assert(stub_cfg.gp[i].value == MCP2221_CONFIG_KEEP);
        assert(stub_cfg.gp[i].direction == MCP2221_CONFIG_KEEP);
        assert(stub_cfg.gp[i].function == MCP2221_CONFIG_KEEP);
    }
    assert(stub_cfg.int_cfg.pos_edge == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.int_cfg.neg_edge == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.int_cfg.clear_flag == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.adc_cfg.vrm == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.adc_cfg.ref_src == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.dac_ref.vrm == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.dac_ref.ref_src == MCP2221_CONFIG_KEEP);
    assert(stub_cfg.dac_val.value == MCP2221_CONFIG_KEEP);
}

static void test_all_mappings(void) {
    static const unsigned duties[] = {0u, 25u, 50u, 75u};
    static const int duty_sel[] = {
        MCP2221_CLK_DUTY_0, MCP2221_CLK_DUTY_25,
        MCP2221_CLK_DUTY_50, MCP2221_CLK_DUTY_75
    };
    static const uint32_t freqs[] = {
        375000u, 750000u, 1500000u, 3000000u,
        6000000u, 12000000u, 24000000u
    };
    static const int freq_sel[] = {
        MCP2221_CLK_FREQ_375kHz, MCP2221_CLK_FREQ_750kHz,
        MCP2221_CLK_FREQ_1_5MHz, MCP2221_CLK_FREQ_3MHz,
        MCP2221_CLK_FREQ_6MHz, MCP2221_CLK_FREQ_12MHz,
        MCP2221_CLK_FREQ_24MHz
    };
    mcp2221_t *dev = (mcp2221_t *)(uintptr_t)1;

    for (size_t d = 0; d < sizeof(duties) / sizeof(duties[0]); d++) {
        for (size_t f = 0; f < sizeof(freqs) / sizeof(freqs[0]); f++) {
            reset_stub();
            assert(mcp2221_clock_config(dev, duties[d], freqs[f]) ==
                   MCP2221_ERR_OK);
            assert(stub_calls == 1);
            assert(stub_cfg.clk_cfg.duty == duty_sel[d]);
            assert(stub_cfg.clk_cfg.div == freq_sel[f]);
            assert_non_clock_fields_keep();
        }
    }
}

static void test_invalid_arguments_and_error_forwarding(void) {
    mcp2221_t *dev = (mcp2221_t *)(uintptr_t)1;
    reset_stub();
    assert(mcp2221_clock_config(NULL, 50u, 375000u) == MCP2221_ERR_INVALID);
    assert(mcp2221_clock_config(dev, 100u, 375000u) == MCP2221_ERR_INVALID);
    assert(mcp2221_clock_config(dev, 50u, 175000u) == MCP2221_ERR_INVALID);
    assert(stub_calls == 0);

    reset_stub();
    stub_result = MCP2221_ERR_USB;
    assert(mcp2221_clock_config(dev, 50u, 375000u) == MCP2221_ERR_USB);
    assert(stub_calls == 1);
}

int main(void) {
    test_all_mappings();
    test_invalid_arguments_and_error_forwarding();
    return 0;
}
