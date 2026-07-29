#ifndef MCP2221_SMBUS_H
#define MCP2221_SMBUS_H

#include <stddef.h>
#include <stdint.h>

#include "error_codes.h"
#include "mcp2221_deprecated.h"

// Forward declaration
typedef struct MCP2221 MCP2221;

#define MCP2221_I2C_SMBUS_BLOCK_MAX 255
/* Legacy macro alias; prefer MCP2221_I2C_SMBUS_BLOCK_MAX. */
#define I2C_SMBUS_BLOCK_MAX MCP2221_I2C_SMBUS_BLOCK_MAX

typedef struct {
	MCP2221 *mcp;
	int owns_mcp;
} SMBus;

typedef SMBus mcp2221_smbus_t;

/* All SMBus functions return MCP_ERR_OK on success or another mcp_err_t value on error.
 * Functions that return data lengths report them via size_t output parameters.
 */

/* Preferred mcp2221_* SMBus names. */
mcp_err_t mcp2221_smbus_init(mcp2221_smbus_t *bus, MCP2221 *existing_mcp, int device_index, uint16_t vid, uint16_t pid,
							  const char *usbserial, uint32_t i2c_speed_hz);
void mcp2221_smbus_close(mcp2221_smbus_t *bus);
mcp_err_t mcp2221_smbus_read_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t *value);
mcp_err_t mcp2221_smbus_write_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t value);
mcp_err_t mcp2221_smbus_read_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *value);
mcp_err_t mcp2221_smbus_write_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t value);
mcp_err_t mcp2221_smbus_read_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t *value);
mcp_err_t mcp2221_smbus_write_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value);
mcp_err_t mcp2221_smbus_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response);
mcp_err_t mcp2221_smbus_read_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length);
mcp_err_t mcp2221_smbus_write_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);
mcp_err_t mcp2221_smbus_block_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length, uint8_t *response, size_t *resp_len);
mcp_err_t mcp2221_smbus_read_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length);
mcp_err_t mcp2221_smbus_write_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length);

/* Legacy smbus_* names; scheduled for removal in a future major version.
 * If existing_mcp is non-NULL, SMBus borrows it and smbus_close() will not close it.
 * Otherwise SMBus opens its own MCP2221 handle and smbus_close() will release it.
 */
MCP2221_DEPRECATED("use mcp2221_smbus_init") mcp_err_t smbus_init(SMBus *bus, MCP2221 *existing_mcp, int device_index, uint16_t vid, uint16_t pid, const char *usbserial,
			   uint32_t i2c_speed_hz);

/* Close SMBus wrapper and release an internally-owned MCP2221 handle, if any. */
MCP2221_DEPRECATED("use mcp2221_smbus_close") void smbus_close(SMBus *bus);

// Basic read/write
MCP2221_DEPRECATED("use mcp2221_smbus_read_byte") mcp_err_t smbus_read_byte(SMBus *bus, uint8_t addr, uint8_t *value);
MCP2221_DEPRECATED("use mcp2221_smbus_write_byte") mcp_err_t smbus_write_byte(SMBus *bus, uint8_t addr, uint8_t value);

MCP2221_DEPRECATED("use mcp2221_smbus_read_byte_data") mcp_err_t smbus_read_byte_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *value);
MCP2221_DEPRECATED("use mcp2221_smbus_write_byte_data") mcp_err_t smbus_write_byte_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t value);

MCP2221_DEPRECATED("use mcp2221_smbus_read_word_data") mcp_err_t smbus_read_word_data(SMBus *bus, uint8_t addr, uint8_t reg, int16_t *value);
MCP2221_DEPRECATED("use mcp2221_smbus_write_word_data") mcp_err_t smbus_write_word_data(SMBus *bus, uint8_t addr, uint8_t reg, int16_t value);

MCP2221_DEPRECATED("use mcp2221_smbus_process_call") mcp_err_t smbus_process_call(SMBus *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response);

// Block operations
MCP2221_DEPRECATED("use mcp2221_smbus_read_block_data") mcp_err_t smbus_read_block_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length);

MCP2221_DEPRECATED("use mcp2221_smbus_write_block_data") mcp_err_t smbus_write_block_data(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);

MCP2221_DEPRECATED("use mcp2221_smbus_block_process_call") mcp_err_t smbus_block_process_call(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length,
							 uint8_t *response, size_t *resp_len);

MCP2221_DEPRECATED("use mcp2221_smbus_read_i2c_block_data") mcp_err_t smbus_read_i2c_block_data(SMBus *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length);

MCP2221_DEPRECATED("use mcp2221_smbus_write_i2c_block_data") mcp_err_t smbus_write_i2c_block_data(SMBus *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);

#endif
