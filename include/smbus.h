#ifndef MCP2221_SMBUS_H
#define MCP2221_SMBUS_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

/* EasyMCP2221 compatibility limit for block helpers.
 * The compatibility layer uses a one-byte length field, so this constant is the
 * maximum payload length accepted by the public helpers. This is intentionally
 * larger than the classic SMBus 32-byte block payload limit.
 */
#define MCP2221_I2C_SMBUS_BLOCK_MAX 255
/* Legacy macro alias; prefer MCP2221_I2C_SMBUS_BLOCK_MAX. */
#define I2C_SMBUS_BLOCK_MAX MCP2221_I2C_SMBUS_BLOCK_MAX

typedef struct mcp2221_smbus {
	mcp2221_t *mcp;
	int owns_mcp;
} mcp2221_smbus_t;

/* All preferred SMBus functions return MCP2221_ERR_OK on success or another
 * mcp2221_error_code_t value on error.
 * Functions that return data lengths report them via size_t output parameters.
 */

/* Preferred mcp2221_* SMBus names. */
mcp2221_error_code_t mcp2221_smbus_init(mcp2221_smbus_t *bus, mcp2221_t *existing_mcp, int device_index, uint16_t vid, uint16_t pid,
							  const char *usbserial, uint32_t i2c_speed_hz);
void mcp2221_smbus_close(mcp2221_smbus_t *bus);
mcp2221_error_code_t mcp2221_smbus_read_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t *value);
mcp2221_error_code_t mcp2221_smbus_write_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t value);
mcp2221_error_code_t mcp2221_smbus_read_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *value);
mcp2221_error_code_t mcp2221_smbus_write_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t value);
mcp2221_error_code_t mcp2221_smbus_read_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t *value);
mcp2221_error_code_t mcp2221_smbus_write_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value);
mcp2221_error_code_t mcp2221_smbus_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response);
/* Block helpers use the EasyMCP2221-compatible payload limit above.
 * Output buffers for read_block_data() and block_process_call() must be able to
 * hold up to MCP2221_I2C_SMBUS_BLOCK_MAX payload bytes.
 */
mcp2221_error_code_t mcp2221_smbus_read_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length);
mcp2221_error_code_t mcp2221_smbus_write_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);
mcp2221_error_code_t mcp2221_smbus_block_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length, uint8_t *response, size_t *resp_len);
mcp2221_error_code_t mcp2221_smbus_read_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length);
mcp2221_error_code_t mcp2221_smbus_write_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length);

#endif
