#include <stdio.h>
#include <stdint.h>
#include "mcp2221.h"
#include "i2c_slave.h"

int main(void)
{
    MCP2221 *dev = mcp2221_open(
        0x04D8, 0x00DD,
        0,          // erstes Gerät
        NULL,       // keine Seriennummer filter
        500,        // read timeout ms
        3,          // retries
        1,          // debug messages
        0           // trace packets
    );

    if (!dev) {
        fprintf(stderr, "MCP2221 not found.\n");
        return 1;
    }

    // Create I2C slave (z.B. EEPROM at 0x50)
    I2C_Slave ee;
    int r = mcp2221_create_i2c_slave(
        dev, &ee,
        0x50,       // I2C addr
        1,          // force (true)
        100000,     // 100 kHz
        2,          // reg_bytes
        "big"
    );
    if (r != MCP_ERR_OK) {
        fprintf(stderr, "Failed to create I2C slave: %d\n", r);
        mcp2221_close(dev);
        return 1;
    }

    // Lese 16 Bytes ab Adresse 0x0000
    uint8_t buf[16];
    r = i2c_slave_read_register(&ee, 0x0000, buf, sizeof(buf), 0, NULL);
    if (r != MCP_ERR_OK) {
        fprintf(stderr, "EEPROM read failed: %d\n", r);
        mcp2221_close(dev);
        return 1;
    }

    printf("EEPROM[0..15]:");
    for (int i = 0; i < 16; ++i)
        printf(" %02X", buf[i]);
    printf("\n");

    mcp2221_close(dev);
    return 0;
}