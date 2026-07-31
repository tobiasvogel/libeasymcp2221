#ifndef MCP2221_SMBUS_H
#define MCP2221_SMBUS_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/* Maximum payload length accepted by the block helper functions.
 * Block lengths are encoded in a single byte.
 */
#define MCP2221_I2C_SMBUS_BLOCK_MAX 255

typedef struct mcp2221_smbus {
	mcp2221_t *mcp;
	int owns_mcp;
} mcp2221_smbus_t;

/* All SMBus functions return MCP2221_ERR_OK on success or another
 * mcp2221_error_code_t value on error.
 * Functions that return data lengths report them via size_t output parameters.
 */

/* SMBus API. */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_init(mcp2221_smbus_t *bus, mcp2221_t *existing_mcp, int device_index, uint16_t vid, uint16_t pid,
							  const char *usbserial, uint32_t i2c_speed_hz);
MCP2221_API void mcp2221_smbus_close(mcp2221_smbus_t *bus);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t *value);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t value);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *value);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t value);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t *value);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response);
/* Block helpers use the EasyMCP2221-compatible payload limit above.
 * Output buffers for read_block_data() and block_process_call() must be able to
 * hold up to MCP2221_I2C_SMBUS_BLOCK_MAX payload bytes.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_block_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length, uint8_t *response, size_t *resp_len);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length);
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length);

MCP2221_END_DECLS
#endif // MCP2221_SMBUS_H
