/**
 * @file mcp2221_smbus.h
 * @brief EasyMCP2221-compatible SMBus helper API.
 */

#ifndef MCP2221_SMBUS_H
#define MCP2221_SMBUS_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Maximum payload length accepted by the SMBus block helpers.
 *
 * Block lengths are encoded in a single byte. The value of 255 follows the
 * EasyMCP2221 compatibility layer and is intentionally larger than the
 * classic 32-byte SMBus block limit.
 */
#define MCP2221_I2C_SMBUS_BLOCK_MAX 255

/**
 * @brief Caller-owned SMBus helper context.
 *
 * Initialize the context with mcp2221_smbus_init() and release it with
 * mcp2221_smbus_close().
 *
 * When initialized with an existing MCP2221 handle, the context borrows that
 * handle. When mcp2221_smbus_init() opens the device itself, the context owns
 * the acquired reference and mcp2221_smbus_close() releases it.
 */
typedef struct mcp2221_smbus {
	/**
	 * @brief MCP2221 device handle used for SMBus transfers.
	 *
	 * After successful initialization this points to the active MCP2221
	 * handle. When mcp2221_smbus_init() receives a non-`NULL`
	 * @p existing_mcp, this is the borrowed handle supplied by the caller.
	 * Otherwise it is the handle opened by mcp2221_smbus_init().
	 *
	 * The member is set to `NULL` before initialization is attempted and by
	 * mcp2221_smbus_close().
	 *
	 * @note Ownership of this handle is described by @ref owns_mcp.
	 */
	mcp2221_t *mcp;

	/**
	 * @brief Ownership flag for @ref mcp.
	 *
	 * A value of 0 means that @ref mcp is borrowed from the caller and must
	 * not be closed by mcp2221_smbus_close(). A nonzero value means that the
	 * SMBus context acquired the handle itself and
	 * mcp2221_smbus_close() releases that reference.
	 *
	 * This flag is maintained by mcp2221_smbus_init() and
	 * mcp2221_smbus_close(); applications should not modify it directly.
	 */
	int owns_mcp;
} mcp2221_smbus_t;

/**
 * @brief Initialize a caller-owned SMBus context.
 *
 * If @p existing_mcp is non-`NULL`, the context borrows that handle and no
 * device is opened. In that case the device-selection parameters and
 * @p i2c_speed_hz are not applied.
 *
 * If @p existing_mcp is `NULL`, the function opens an MCP2221 device using
 * mcp2221_open_simple(). An @p i2c_speed_hz value of 0 selects 100 kHz;
 * values greater than MCP2221_I2C_SPEED_MAX_HZ are invalid.
 *
 * @param[out] bus Caller-owned SMBus context to initialize. Must not be `NULL`.
 * @param[in] existing_mcp Existing MCP2221 handle to borrow, or `NULL` to open
 *                         a device for this context.
 * @param[in] device_index Zero-based device index used when opening a device.
 * @param[in] vid USB vendor ID used when opening a device.
 * @param[in] pid USB product ID used when opening a device.
 * @param[in] usbserial USB serial number to match when opening a device, or
 *                      `NULL` to ignore the serial number.
 * @param[in] i2c_speed_hz Requested I2C clock frequency in hertz when opening
 *                         a device. Zero selects 100 kHz; values greater than
 *                         MCP2221_I2C_SPEED_MAX_HZ are invalid.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for a `NULL` bus or
 *         an out-of-range speed when opening a device, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_smbus_close()
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_init(mcp2221_smbus_t *bus, mcp2221_t *existing_mcp, int device_index, uint16_t vid, uint16_t pid,
							  const char *usbserial, uint32_t i2c_speed_hz);

/**
 * @brief Close an SMBus context.
 *
 * If the context opened its MCP2221 handle during mcp2221_smbus_init(), one
 * reference is released with mcp2221_close(). A borrowed handle is not closed.
 * The context is cleared in either case.
 *
 * @param[in,out] bus SMBus context to close, or `NULL`.
 *
 * @note Passing `NULL` is allowed and has no effect.
 */
MCP2221_API void mcp2221_smbus_close(mcp2221_smbus_t *bus);

/**
 * @brief Read one byte directly from an SMBus target.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[out] value Receives the byte read from the target.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t *value);

/**
 * @brief Write one byte directly to an SMBus target.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] value Byte to write.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_byte(mcp2221_smbus_t *bus, uint8_t addr, uint8_t value);

/**
 * @brief Read one byte from an SMBus command/register.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[out] value Receives the byte read from the target.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * @brief Write one byte to an SMBus command/register.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[in] value Byte to write.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_byte_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t value);

/**
 * @brief Read a 16-bit word from an SMBus command/register.
 *
 * The two data bytes are decoded least-significant byte first to match the
 * EasyMCP2221 SMBus compatibility behavior.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[out] value Receives the decoded signed 16-bit value.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t *value);

/**
 * @brief Write a 16-bit word to an SMBus command/register.
 *
 * The value is encoded least-significant byte first.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[in] value Signed 16-bit value to write.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_word_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value);

/**
 * @brief Perform an SMBus process call.
 *
 * Writes a command byte and a little-endian 16-bit value without a STOP
 * condition, then reads a little-endian 16-bit response using a repeated
 * START.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[in] value Signed 16-bit value to send.
 * @param[out] response Receives the signed 16-bit response.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, int16_t value, int16_t *response);

/**
 * @brief Read an SMBus length-prefixed block.
 *
 * Reads one length byte followed by up to MCP2221_I2C_SMBUS_BLOCK_MAX payload
 * bytes. The caller-provided @p buffer must be large enough for the maximum
 * payload.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[out] buffer Buffer receiving the payload. Must hold at least
 *                    MCP2221_I2C_SMBUS_BLOCK_MAX bytes.
 * @param[out] length Receives the payload length reported by the target.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID if the reported
 *         length is invalid, or another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t *length);

/**
 * @brief Write an SMBus length-prefixed block.
 *
 * The function writes the command byte, one length byte, and the supplied
 * payload.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[in] data Payload bytes to write. Must not be `NULL`.
 * @param[in] length Payload length. May be 0 and must not exceed
 *                   MCP2221_I2C_SMBUS_BLOCK_MAX.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data, size_t length);

/**
 * @brief Perform an SMBus block process call.
 *
 * Writes the command byte, a one-byte payload length, and the supplied payload
 * without a STOP condition. It then reads a length-prefixed response using a
 * repeated START.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[in] data Payload bytes to send. Must not be `NULL`.
 * @param[in] length Payload length. May be 0 and must not exceed
 *                   MCP2221_I2C_SMBUS_BLOCK_MAX.
 * @param[out] response Buffer receiving the response payload. Must hold at
 *                      least MCP2221_I2C_SMBUS_BLOCK_MAX bytes.
 * @param[out] resp_len Receives the response payload length.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID if an argument or the
 *         reported response length is invalid, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_block_process_call(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length, uint8_t *response, size_t *resp_len);

/**
 * @brief Read a fixed-length I2C block from an SMBus command/register.
 *
 * Unlike mcp2221_smbus_read_block_data(), this helper does not consume a
 * length prefix from the target.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[out] buffer Buffer receiving the requested bytes.
 * @param[in] length Number of bytes requested. Must be from 1 through
 *                   MCP2221_I2C_SMBUS_BLOCK_MAX.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_read_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, uint8_t *buffer, size_t length);

/**
 * @brief Write a fixed-length I2C block to an SMBus command/register.
 *
 * Unlike mcp2221_smbus_write_block_data(), this helper does not add a payload
 * length byte.
 *
 * @param[in] bus Initialized SMBus context.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] reg SMBus command/register byte.
 * @param[in] data Bytes to write. Must not be `NULL`.
 * @param[in] length Number of bytes to write. May be 0 and must not exceed
 *                   MCP2221_I2C_SMBUS_BLOCK_MAX.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_smbus_write_i2c_block_data(mcp2221_smbus_t *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
									   size_t length);

MCP2221_END_DECLS
#endif // MCP2221_SMBUS_H
