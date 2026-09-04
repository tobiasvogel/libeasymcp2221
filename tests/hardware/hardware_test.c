#ifndef _WIN32
#define _POSIX_C_SOURCE 199309L
#endif

#include "hardware_test.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define HW_DEFAULT_VID 0x04d8u
#define HW_DEFAULT_PID 0x00ddu

static int parse_long_env(const char *name, long min_value, long max_value,
                          long default_value, long *out_value)
{
    const char *text;
    char *end;
    long value;

    if (out_value == NULL) {
        return HW_TEST_FAILED;
    }

    text = getenv(name);
    if (text == NULL || text[0] == '\0') {
        *out_value = default_value;
        return HW_TEST_OK;
    }

    errno = 0;
    end = NULL;
    value = strtol(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' ||
        value < min_value || value > max_value) {
        fprintf(stderr, "Invalid %s value: %s\n", name, text);
        return HW_TEST_FAILED;
    }

    *out_value = value;
    return HW_TEST_OK;
}

int hw_test_load_config(hw_test_config_t *cfg)
{
    long vid;
    long pid;
    long devnum;
    long eeprom_addr;
    const char *serial;

    if (cfg == NULL) {
        return HW_TEST_FAILED;
    }

    if (parse_long_env("LIBEASYMCP2221_HW_VID", 0, 0xffff,
                       HW_DEFAULT_VID, &vid) != HW_TEST_OK ||
        parse_long_env("LIBEASYMCP2221_HW_PID", 0, 0xffff,
                       HW_DEFAULT_PID, &pid) != HW_TEST_OK ||
        parse_long_env("LIBEASYMCP2221_HW_DEVNUM", 0, INT_MAX,
                       0, &devnum) != HW_TEST_OK ||
        parse_long_env("LIBEASYMCP2221_HW_EEPROM_ADDR", 0x03, 0x77,
                       0x50, &eeprom_addr) != HW_TEST_OK) {
        return HW_TEST_FAILED;
    }

    serial = getenv("LIBEASYMCP2221_HW_SERIAL");
    if (serial != NULL && serial[0] == '\0') {
        serial = NULL;
    }

    cfg->vid = (uint16_t)vid;
    cfg->pid = (uint16_t)pid;
    cfg->devnum = (int)devnum;
    cfg->serial = serial;
    cfg->eeprom_addr = (uint8_t)eeprom_addr;
    return HW_TEST_OK;
}

void hw_test_print_error(const char *operation, mcp2221_error_code_t rc)
{
    fprintf(stderr, "%s failed: %s (%d)\n",
            operation, mcp2221_error_code_to_string(rc), (int)rc);
}

int hw_test_open(const hw_test_config_t *cfg, mcp2221_t **out_dev)
{
    mcp2221_error_code_t rc;

    if (cfg == NULL || out_dev == NULL) {
        return HW_TEST_FAILED;
    }

    /*
     * Do not use mcp2221_open_simple() here. The Python reference fixture may
     * intentionally hold SCL/SDA low through GP0/GP1 at process start.
     * I2C tests first release those fixture lines and only then configure the
     * bus speed.
     */
    *out_dev = NULL;
    rc = mcp2221_open(cfg->vid, cfg->pid, cfg->devnum, cfg->serial,
                      -1, 1, 0, 0, out_dev);
    if (rc == MCP2221_ERR_NOT_FOUND) {
        printf("SKIP: no matching MCP2221 found "
               "(VID=0x%04x PID=0x%04x devnum=%d)\n",
               (unsigned int)cfg->vid, (unsigned int)cfg->pid, cfg->devnum);
        return HW_TEST_SKIPPED;
    }

    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("mcp2221_open", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

int hw_test_safe_state(mcp2221_t *dev)
{
    const mcp2221_pin_functions_t safe = {
        {
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN,
            MCP2221_PIN_FUNC_GPIO_IN
        },
        {0, 0, 0, 0}
    };
    mcp2221_error_code_t rc;

    rc = mcp2221_pin_set_functions(dev, &safe);
    if (rc != MCP2221_ERR_OK) {
        hw_test_print_error("setting safe GPIO state", rc);
        return HW_TEST_FAILED;
    }

    return HW_TEST_OK;
}

void hw_test_sleep_ms(unsigned int milliseconds)
{
#ifdef _WIN32
    Sleep((DWORD)milliseconds);
#else
    struct timespec delay;

    delay.tv_sec = (time_t)(milliseconds / 1000u);
    delay.tv_nsec = (long)(milliseconds % 1000u) * 1000000L;

    while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {
    }
#endif
}
