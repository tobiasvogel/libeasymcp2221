#include "hardware_test.h"

#include <stdio.h>

int main(void)
{
    hw_test_config_t cfg;
    mcp2221_t *dev;
    int result;

    if (hw_test_load_config(&cfg) != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    result = hw_test_open(&cfg, &dev);
    if (result != HW_TEST_OK) {
        return result;
    }

    printf("Opened MCP2221 (VID=0x%04x PID=0x%04x devnum=%d",
           (unsigned int)cfg.vid, (unsigned int)cfg.pid, cfg.devnum);
    if (cfg.serial != NULL) {
        printf(" serial=%s", cfg.serial);
    }
    printf(")\n");

    /*
     * Match the Python test suite's tearDown() policy: leave the GP pins in a
     * non-driving state even though this smoke test itself does not need GPIO.
     */
    result = hw_test_safe_state(dev);
    mcp2221_close(dev);

    if (result != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    printf("PASS: device open/close and safe-state smoke test\n");
    return HW_TEST_OK;
}
