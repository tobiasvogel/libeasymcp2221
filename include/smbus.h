#ifndef SMBUS_H
#define SMBUS_H

#include <stddef.h>
#include <stdint.h>

// Forward declaration
typedef struct MCP2221 MCP2221;

#define I2C_SMBUS_BLOCK_MAX 255

typedef struct {
	MCP2221 *mcp;
} SMBus;

// Constructor-like init
int smbus_init(SMBus *bus, MCP2221 *existing_mcp, int device_index, uint16_t vid, uint16_t pid, const char *usbserial,
			   uint32_t clock_hz);

// Basic read/write
int smbus_read_byte(SMBus *bus, uint8_t addr, uint8_t *value);
int smbus_write_byte(SMBus *bus, uint8_t addr, uint8_t value);

int smbus_read_byte_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *value);
int smbus_write_byte_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t value);

int smbus_read_word_data(SMBus *bus, uint8_t addr, uint8_t reg, int16_t *value);
int smbus_write_word_data(SMBus *bus, uint8_t addr, uint8_t reg, int16_t value);

int smbus_process_call(SMBus *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response);

// Block operations
int smbus_read_block_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length);

int smbus_write_block_data(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);

int smbus_block_process_call(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length,
							 uint8_t *response, size_t *resp_len);

int smbus_read_i2c_block_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length);

int smbus_write_i2c_block_data(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);

#endif
