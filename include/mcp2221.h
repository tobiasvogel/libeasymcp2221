/**
 * @file mcp2221.h
 * @brief Core MCP2221 device and I2C master API.
 */

#ifndef MCP2221_H
#define MCP2221_H

#include <stddef.h>
#include <stdint.h>
#include "mcp2221_export.h"
#include "mcp2221_error_codes.h"
#include "mcp2221_errors.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Opaque MCP2221 device handle.
 *
 * Device handles are created by the mcp2221_open*() family and released with
 * mcp2221_close().
 *
 * @warning Operations on the same device handle are not internally
 *          serialized. Applications that access one mcp2221_t from multiple
 *          threads must provide their own synchronization around I2C, GPIO,
 *          flash, and other device operations.
 */
typedef struct mcp2221_device mcp2221_t;

/**
 * @brief Caller-owned high-level I2C slave context.
 *
 * The structure is defined in mcp2221_i2c_slave.h so callers may allocate it
 * statically, on the stack, or as part of another structure. Initialize it
 * with mcp2221_i2c_slave_init().
 */
typedef struct mcp2221_i2c_slave mcp2221_i2c_slave_t;

/**
 * @brief Snapshot of the MCP2221 I2C engine status.
 */
typedef struct {
	uint16_t rlen;       /**< Requested transfer length reported by the device. */
	uint16_t txlen;      /**< Number of bytes transmitted by the I2C engine. */
	uint8_t div;         /**< MCP2221 I2C clock-divider register value. */
	uint8_t ack;         /**< Raw ACK-status bit mask (bit 6), either 0 or 0x40. */
	uint8_t st;          /**< Raw MCP2221 internal I2C state code. */
	uint8_t scl;         /**< Sampled SCL line level, 0 or 1. */
	uint8_t sda;         /**< Sampled SDA line level, 0 or 1. */
	uint8_t confused;    /**< Nonzero when the EasyMCP2221 compatibility heuristic
	                      * considers the I2C engine state inconsistent. */
	uint8_t initialized; /**< Nonzero when the compatibility heuristic considers
	                      * the I2C engine initialized. */
} mcp2221_i2c_status_t;

/**
 * @brief I2C transfer kind.
 */
typedef enum {
	MCP2221_I2C_KIND_NORMAL = 0,         /**< Normal transfer. */
	MCP2221_I2C_KIND_REPEATED_START = 1, /**< Transfer using a repeated START. */
	MCP2221_I2C_KIND_NO_STOP = 2         /**< Write without a STOP condition. */
} mcp2221_i2c_kind_t;

/**
 * @brief Open an MCP2221 device.
 *
 * Opens the matching device or acquires another reference to an already open
 * matching device.
 *
 * When a matching device is already open, the existing device context is
 * returned and its reference count is increased. In that case the
 * @p usb_read_timeout_ms, @p cmd_retries, @p debug_messages, and
 * @p trace_packets values from this call do not replace the settings stored
 * in the existing context.
 *
 * Each successful call must be matched by a call to mcp2221_close().
 *
 * @param[in] vid USB vendor ID. The MCP2221 default is 0x04D8.
 * @param[in] pid USB product ID. The MCP2221 default is 0x00DD.
 * @param[in] devnum Device index when multiple matching devices are present.
 *                   Use 0 for the first matching device.
 * @param[in] usbserial USB serial number to match, or `NULL` to ignore the USB
 *                      serial number.
 * @param[in] usb_read_timeout_ms USB read timeout in milliseconds. Values
 *                                less than or equal to zero disable the USB
 *                                read timeout.
 * @param[in] cmd_retries Number of retries used by operations whose commands
 *                        are safe to repeat. Negative values are treated as 0.
 *                        Mutating commands are not generically retried because
 *                        a lost response does not prove that the device did
 *                        not execute the command.
 * @param[in] debug_messages Nonzero to enable debug messages.
 * @param[in] trace_packets Nonzero to enable USB packet tracing.
 * @param[out] out_dev Receives the device handle on success. Must not be
 *                     `NULL`. `*out_dev` is set to `NULL` before opening is
 *                     attempted.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_close()
 */
MCP2221_API mcp2221_error_code_t mcp2221_open(
	uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
	int usb_read_timeout_ms, int cmd_retries, int debug_messages,
	int trace_packets, mcp2221_t **out_dev);

/**
 * @brief Open an MCP2221 device, optionally scanning the flash serial number.
 *
 * This variant can scan the MCP2221 flash serial number when the USB serial is
 * not enumerated.
 *
 * @param[in] vid USB vendor ID.
 * @param[in] pid USB product ID.
 * @param[in] devnum Device index when multiple matching devices are present.
 * @param[in] usbserial USB serial number to match, or `NULL` to ignore it.
 * @param[in] usb_read_timeout_ms USB read timeout in milliseconds. Values
 *                                less than or equal to zero disable the USB
 *                                read timeout.
 * @param[in] cmd_retries Number of retries used by operations whose commands
 *                        are safe to repeat. Negative values are treated as 0.
 *                        Mutating commands are not generically retried because
 *                        a lost response does not prove that the device did
 *                        not execute the command.
 * @param[in] debug_messages Nonzero to enable debug messages.
 * @param[in] trace_packets Nonzero to enable USB packet tracing.
 * @param[in] scan_serial Nonzero to enable flash-serial scanning.
 * @param[out] out_dev Receives the device handle on success. Must not be
 *                     `NULL`. `*out_dev` is set to `NULL` before opening is
 *                     attempted.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_open()
 */
MCP2221_API mcp2221_error_code_t mcp2221_open_scan(
	uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
	int usb_read_timeout_ms, int cmd_retries, int debug_messages,
	int trace_packets, int scan_serial, mcp2221_t **out_dev);

/**
 * @brief Open and initialize an MCP2221 using an EasyMCP2221-style setup.
 *
 * The supplied I2C clock speed is applied during initialization.
 *
 * @param[in] vid USB vendor ID.
 * @param[in] pid USB product ID.
 * @param[in] devnum Device index when multiple matching devices are present.
 * @param[in] usbserial USB serial number to match, or `NULL` to ignore it.
 * @param[in] i2c_speed_hz Requested I2C clock frequency in hertz. Values
 *                         greater than MCP2221_I2C_SPEED_MAX_HZ are invalid;
 *                         values less than or equal to zero select 100 kHz.
 * @param[out] out_dev Receives the device handle on success. Must not be
 *                     `NULL`. `*out_dev` is set to `NULL` before opening is
 *                     attempted.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_open_simple(
	uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
	int i2c_speed_hz, mcp2221_t **out_dev);

/**
 * @brief Open and initialize an MCP2221 with optional flash-serial scanning.
 *
 * @param[in] vid USB vendor ID.
 * @param[in] pid USB product ID.
 * @param[in] devnum Device index when multiple matching devices are present.
 * @param[in] usbserial USB serial number to match, or `NULL` to ignore it.
 * @param[in] i2c_speed_hz Requested I2C clock frequency in hertz. Values
 *                         greater than MCP2221_I2C_SPEED_MAX_HZ are invalid;
 *                         values less than or equal to zero select 100 kHz.
 * @param[in] scan_serial Nonzero to enable flash-serial scanning.
 * @param[out] out_dev Receives the device handle on success. Must not be
 *                     `NULL`. `*out_dev` is set to `NULL` before opening is
 *                     attempted.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_open_simple()
 * @see mcp2221_open_scan()
 */
MCP2221_API mcp2221_error_code_t mcp2221_open_simple_scan(
	uint16_t vid, uint16_t pid, int devnum, const char *usbserial,
	int i2c_speed_hz, int scan_serial, mcp2221_t **out_dev);

/**
 * @brief Close an MCP2221 device handle.
 *
 * Releases one reference acquired through the mcp2221_open*() family.
 *
 * @param[in] dev Device handle to close, or `NULL`.
 *
 * @note Passing `NULL` is allowed and has no effect.
 *
 * @see mcp2221_open()
 */
MCP2221_API void mcp2221_close(mcp2221_t *dev);

/**
 * @brief Send a raw MCP2221 command.
 *
 * Commands shorter than MCP2221_PACKET_SIZE are padded internally before
 * transmission.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] buf Command bytes to send.
 * @param[in] len Number of command bytes. Must be between 1 and
 *                MCP2221_PACKET_SIZE.
 * @param[out] response Optional buffer receiving the complete
 *                      MCP2221_PACKET_SIZE-byte response. When non-`NULL`,
 *                      the buffer must provide room for at least
 *                      MCP2221_PACKET_SIZE bytes.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_send_cmd(mcp2221_t *dev, const uint8_t *buf, size_t len, uint8_t *response /* 64-Byte Buffer */);

/**
 * @brief Set the I2C bus clock frequency.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] i2c_speed_hz Requested I2C clock frequency in hertz. Must be
 *                         greater than zero, no greater than
 *                         MCP2221_I2C_SPEED_MAX_HZ, and representable by the
 *                         MCP2221 8-bit I2C clock divider.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_set_speed(mcp2221_t *dev, uint32_t i2c_speed_hz);

/**
 * @brief Write data to an I2C device with an explicit transfer timeout.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] addr 7-bit I2C device address.
 * @param[in] data Data to write.
 * @param[in] len Number of bytes to write. Must be between 1 and
 *                MCP2221_I2C_TRANSFER_MAX.
 * @param[in] kind I2C transfer kind.
 * @param[in] i2c_timeout_ms Per-chunk/progress watchdog timeout in
 *                           milliseconds. Values less than or equal to zero
 *                           select a 20 ms watchdog. This is not a hard
 *                           timeout for the complete multi-chunk transfer.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @note @p kind may be MCP2221_I2C_KIND_NORMAL,
 *       MCP2221_I2C_KIND_REPEATED_START, or
 *       MCP2221_I2C_KIND_NO_STOP.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_write_ex(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len,
								 mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

/**
 * @brief Write data to an I2C device using the default transfer timeout.
 *
 * Uses the device's configured USB read timeout as the I2C watchdog when it is
 * positive; otherwise a 20 ms watchdog is used.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] addr 7-bit I2C device address.
 * @param[in] data Data to write.
 * @param[in] len Number of bytes to write. Must be between 1 and
 *                MCP2221_I2C_TRANSFER_MAX.
 * @param[in] kind I2C transfer kind.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_i2c_write_ex()
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_write_simple(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len, mcp2221_i2c_kind_t kind);

/**
 * @brief Read data from an I2C device with an explicit transfer timeout.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] addr 7-bit I2C device address.
 * @param[out] data Buffer receiving the read data.
 * @param[in] len Number of bytes to read. Must be between 1 and
 *                MCP2221_I2C_TRANSFER_MAX.
 * @param[in] kind I2C transfer kind.
 * @param[in] i2c_timeout_ms Progress watchdog timeout in milliseconds.
 *                           Values less than or equal to zero select a 20 ms
 *                           watchdog. The watchdog is restarted only when
 *                           read data is received, so it is not a hard
 *                           timeout for the complete transfer.
 *
 * @return MCP2221_ERR_OK on success. A documented GET_I2C_DATA error
 *         indication is returned as MCP2221_ERR_I2C; a malformed
 *         GET_I2C_DATA response is returned as MCP2221_ERR_PROTOCOL. Other
 *         failures return the corresponding mcp2221_error_code_t value.
 *
 * @note @p kind may be MCP2221_I2C_KIND_NORMAL or
 *       MCP2221_I2C_KIND_REPEATED_START.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_read_ex(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len,
								mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

/**
 * @brief Read data from an I2C device using the default transfer timeout.
 *
 * Uses the device's configured USB read timeout as the I2C watchdog when it is
 * positive; otherwise a 20 ms watchdog is used.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] addr 7-bit I2C device address.
 * @param[out] data Buffer receiving the read data.
 * @param[in] len Number of bytes to read. Must be between 1 and
 *                MCP2221_I2C_TRANSFER_MAX.
 * @param[in] kind I2C transfer kind.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 *
 * @see mcp2221_i2c_read_ex()
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_read_simple(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len, mcp2221_i2c_kind_t kind);

/**
 * @brief Read the current MCP2221 I2C engine status.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[out] st Receives the I2C engine status snapshot.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_status(mcp2221_t *dev, mcp2221_i2c_status_t *st);

/**
 * @brief Release the MCP2221 I2C engine.
 *
 * @param[in] dev Open MCP2221 device handle.
 *
 * @return MCP2221_ERR_OK on success, or another
 *         mcp2221_error_code_t value on failure.
 */
MCP2221_API mcp2221_error_code_t mcp2221_i2c_release(mcp2221_t *dev);

MCP2221_END_DECLS
#endif	// MCP2221_H
