#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mcp2221_constants.h"
#include "mcp2221_i2c_slave.h"
#include "mcp2221.h"

int main(void)
{
    mcp2221_t *dev = mcp2221_open(
        MCP2221_DEV_DEFAULT_VID, MCP2221_DEV_DEFAULT_PID,
        0,          // first device
        NULL,       // don't use serial
        500,        // USB read timeout in ms
        3,          // command retries
        1,          // debug messages
        0           // trace packets
    );

    if (!dev) {
        fprintf(stderr, "MCP2221 not found.\n");
        return EXIT_FAILURE;
    }

    // Initialize caller-owned I2C slave context (i.e. EEPROM at 0x50)
    mcp2221_i2c_slave_t ee;
    mcp2221_error_code_t err = mcp2221_i2c_slave_init(
        &ee, dev,
        0x50,       // I2C addr
        1,          // force (true)
        100000,     // 100 kHz
        2,          // reg_bytes
        "big"
    );
    if (err != MCP2221_ERR_OK) {
        fprintf(stderr, "Failed to create I2C slave: %s\n",
                mcp2221_error_code_to_string(err));
        mcp2221_close(dev);
        return EXIT_FAILURE;
    }

    // Read 16 Bytes from Address 0x0000
    uint8_t buf[16];
    err = mcp2221_i2c_slave_read_register(&ee, 0x0000, buf, sizeof(buf), 0, NULL);
    if (err != MCP2221_ERR_OK) {
        fprintf(stderr, "EEPROM read failed: %s\n",
                mcp2221_error_code_to_string(err));
        mcp2221_close(dev);
        return EXIT_FAILURE;
    }

    printf("EEPROM[0..15]:");
    for (int i = 0; i < 16; ++i)
        printf(" %02X", buf[i]);
    printf("\n");

    mcp2221_close(dev);
    return EXIT_SUCCESS;
}