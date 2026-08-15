/**
 * @file mcp2221_i2c_slave.h
 * @brief High-level helpers for communicating with an I2C target device.
 */

#ifndef MCP2221_I2C_SLAVE_H
#define MCP2221_I2C_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Register-address byte order used by I2C slave register helpers.
 */
typedef enum {
	/**
	 * Use the default byte order.
	 *
	 * During mcp2221_i2c_slave_init(), this resolves to big endian. For
	 * per-operation register helpers it resolves to the byte order stored in
	 * the slave context.
	 */
	MCP2221_I2C_BYTE_ORDER_DEFAULT = -1,

	/** Encode register addresses most-significant byte first. */
	MCP2221_I2C_BYTE_ORDER_BIG = 0,

	/** Encode register addresses least-significant byte first. */
	MCP2221_I2C_BYTE_ORDER_LITTLE = 1
} mcp2221_i2c_byte_order_t;

/**
 * @brief Caller-owned I2C target context.
 *
 * This is a public value type, not an opaque handle. Applications may allocate
 * it statically, on the stack, or as part of another structure. Initialize it
 * with mcp2221_i2c_slave_init() before using any other slave helper.
 *
 * The context borrows the underlying mcp2221_t handle; destroying or
 * overwriting the context does not close the MCP2221 device.
 */
struct mcp2221_i2c_slave {
	mcp2221_t *mcp;                           /**< Borrowed MCP2221 device handle. */
	uint8_t addr;                             /**< 7-bit I2C target address. */
	int reg_bytes;                            /**< Default register-address width, from 1 to 4 bytes. */
	mcp2221_i2c_byte_order_t reg_byteorder;   /**< Default register-address byte order. */
};

/**
 * @brief Initialize a caller-owned I2C target context.
 *
 * Configures the MCP2221 I2C clock and, unless @p force is nonzero, verifies
 * that the target acknowledges its address. The context is committed only
 * after all validation and setup steps succeed.
 *
 * On failure, @p slave is left invalid with `slave->mcp == NULL`.
 *
 * @param[out] slave Caller-owned context to initialize. Must not be `NULL`.
 * @param[in] mcp Open MCP2221 device handle.
 * @param[in] addr 7-bit I2C target address.
 * @param[in] force Nonzero to skip the initial presence check.
 * @param[in] i2c_speed_hz Requested I2C clock frequency in hertz.
 * @param[in] reg_bytes Default register-address width. Values less than or
 *                      equal to 0 select 1 byte; valid explicit widths are
 *                      1 through 4 bytes.
 * @param[in] reg_byteorder Default register-address byte order.
 *                          MCP2221_I2C_BYTE_ORDER_DEFAULT selects big endian.
 *
 * @return MCP2221_ERR_OK on success. Returns MCP2221_ERR_NOT_ACK when the
 *         presence check is enabled and the target does not acknowledge, or
 *         another mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_slave_init(mcp2221_i2c_slave_t *slave, mcp2221_t *mcp, uint8_t addr, int force, uint32_t i2c_speed_hz,
						   int reg_bytes, mcp2221_i2c_byte_order_t reg_byteorder);

/**
 * @brief Check whether the configured I2C target acknowledges its address.
 *
 * An address NACK is treated as a successful presence check with
 * `*is_present == 0`. Transport, timeout, and other I2C failures are returned
 * to the caller.
 *
 * @param[in] slave Initialized I2C target context.
 * @param[out] is_present Set to 1 when the target acknowledges and 0 when it
 *                        NACKs its address. Must not be `NULL`.
 *
 * @return MCP2221_ERR_OK when the presence check itself completed, including
 *         the NACK case, or another mcp2221_error_code_t value on failure.
 *
 * @note The presence probe uses a one-byte normal I2C read with a 50 ms
 *       transfer timeout.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_slave_check_present(mcp2221_i2c_slave_t *slave, int *is_present);

/**
 * @brief Boolean-style convenience presence check.
 *
 * @param[in] slave Initialized I2C target context.
 *
 * @return 1 when the target acknowledges its address, otherwise 0.
 *
 * @warning A return value of 0 does not distinguish an address NACK from
 *          transport, timeout, or other errors. Use
 *          mcp2221_i2c_slave_check_present() when the distinction matters.
 */
MCP2221_API int mcp2221_i2c_slave_is_present(mcp2221_i2c_slave_t *slave);

/**
 * @brief Read bytes starting at a target register.
 *
 * Encodes @p reg using the selected register width and byte order, writes the
 * register address without a STOP condition, then reads the requested data
 * using a repeated START.
 *
 * @param[in] slave Initialized I2C target context.
 * @param[in] reg Register address to read from.
 * @param[out] buffer Buffer receiving the data.
 * @param[in] length Number of data bytes to read. Must be from 1 to 256.
 * @param[in] reg_bytes Register-address width for this operation. Values less
 *                      than or equal to 0 use the context default; valid
 *                      explicit widths are 1 through 4 bytes.
 * @param[in] reg_byteorder Byte order for this operation.
 *                          MCP2221_I2C_BYTE_ORDER_DEFAULT uses the context
 *                          default.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @note The register write and repeated-start read each use a 50 ms transfer
 *       timeout.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_slave_read_register(mcp2221_i2c_slave_t *slave, uint32_t reg, uint8_t *buffer, size_t length,
									int reg_bytes, mcp2221_i2c_byte_order_t reg_byteorder);

/**
 * @brief Read bytes directly from the configured I2C target.
 *
 * @param[in] slave Initialized I2C target context.
 * @param[out] buffer Buffer receiving the data.
 * @param[in] length Number of bytes to read. Must be from 1 to 256.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @note Uses a normal I2C read with a 50 ms transfer timeout.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_slave_read(mcp2221_i2c_slave_t *slave, uint8_t *buffer, size_t length);

/**
 * @brief Write a register address followed by optional data.
 *
 * Encodes @p reg using the selected register width and byte order and sends
 * the encoded register address followed by @p data in one normal I2C write.
 *
 * @param[in] slave Initialized I2C target context.
 * @param[in] reg Register address to write.
 * @param[in] data Data bytes to append after the register address. May be
 *                 `NULL` when @p length is 0.
 * @param[in] length Number of data bytes to write. May be 0 and must not
 *                   exceed 256.
 * @param[in] reg_bytes Register-address width for this operation. Values less
 *                      than or equal to 0 use the context default; valid
 *                      explicit widths are 1 through 4 bytes.
 * @param[in] reg_byteorder Byte order for this operation.
 *                          MCP2221_I2C_BYTE_ORDER_DEFAULT uses the context
 *                          default.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @note Uses a normal I2C write with a 50 ms transfer timeout.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_slave_write_register(mcp2221_i2c_slave_t *slave, uint32_t reg, const uint8_t *data, size_t length,
									 int reg_bytes, mcp2221_i2c_byte_order_t reg_byteorder);

/**
 * @brief Write bytes directly to the configured I2C target.
 *
 * @param[in] slave Initialized I2C target context.
 * @param[in] data Data to write.
 * @param[in] length Number of bytes to write. Must be from 1 to 256.
 *
 * @return MCP2221_ERR_OK on success, or another mcp2221_error_code_t value
 *         on failure.
 *
 * @note Uses a normal I2C write with a 50 ms transfer timeout.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_slave_write(mcp2221_i2c_slave_t *slave, const uint8_t *data, size_t length);

MCP2221_END_DECLS
#endif	// MCP2221_I2C_SLAVE_H
