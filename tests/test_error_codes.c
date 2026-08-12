#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "mcp2221_errors.h"

typedef struct {
    mcp2221_error_code_t code;
    const char *name;
} error_name_case_t;

int main(void) {
    static const error_name_case_t cases[] = {
        {MCP2221_ERR_OK, "OK"},
        {MCP2221_ERR_USB, "USBError"},
        {MCP2221_ERR_TIMEOUT, "TimeoutError"},
        {MCP2221_ERR_NOT_ACK, "NotAckError"},
        {MCP2221_ERR_LOW_SCL, "LowSCLError"},
        {MCP2221_ERR_LOW_SDA, "LowSDAError"},
        {MCP2221_ERR_INVALID, "InvalidError"},
        {MCP2221_ERR_I2C, "GenericI2CError"},
        {MCP2221_ERR_FLASH_WRITE, "FlashWriteError"},
        {MCP2221_ERR_FLASH_PASSWD, "FlashPasswordError"},
        {MCP2221_ERR_GPIO_MODE, "GPIOModeError"},
        {MCP2221_ERR_GENERIC, "GenericError"},
        {MCP2221_ERR_I2C_SHORT_READ, "I2CShortReadError"},
        {MCP2221_ERR_FLASH_READ, "FlashReadError"},
        {MCP2221_ERR_NOT_FOUND, "NotFoundError"},
        {MCP2221_ERR_NO_MEMORY, "NoMemoryError"},
        {MCP2221_ERR_ACCESS, "AccessError"},
        {MCP2221_ERR_BUSY, "BusyError"},
        {MCP2221_ERR_USB_INIT, "USBInitError"},
        {MCP2221_ERR_USB_ENUM, "USBEnumerationError"},
        {MCP2221_ERR_USB_OPEN, "USBOpenError"},
        {MCP2221_ERR_USB_CLAIM, "USBClaimError"},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        assert(strcmp(
            mcp2221_error_code_to_string(cases[i].code),
            cases[i].name) == 0);
    }

    assert(strcmp(
        mcp2221_error_code_to_string((mcp2221_error_code_t)-999),
        "GenericError") == 0);

    return 0;
}