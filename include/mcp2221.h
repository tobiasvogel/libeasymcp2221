#ifndef MCP2221_H
#define MCP2221_H

#include <stddef.h>
#include <stdint.h>
#include "mcp2221_error_codes.h"
#include "mcp2221_errors.h"

/* Preferred opaque device type. */
typedef struct mcp2221_device mcp2221_t;

/* Caller-owned high-level I2C slave context.
 *
 * The structure is defined in i2c_slave.h so it can be allocated by callers,
 * including on the stack. Initialize it with mcp2221_i2c_slave_init().
 */
typedef struct mcp2221_i2c_slave mcp2221_i2c_slave_t;

/* Snapshot of the MCP2221 I2C engine status.
 *
 * Field names are retained for 1.x source and ABI compatibility. Their exact
 * meanings are documented below; clearer names may be introduced in 2.0.
 */
typedef struct {
	uint16_t rlen;       /* Requested transfer length reported by the device. */
	uint16_t txlen;      /* Number of bytes transmitted by the I2C engine. */
	uint8_t div;         /* MCP2221 I2C clock-divider register value. */
	uint8_t ack;         /* Raw ACK-status bit mask (bit 6), either 0 or 0x40. */
	uint8_t st;          /* Raw MCP2221 internal I2C state code. */
	uint8_t scl;         /* Sampled SCL line level, 0 or 1. */
	uint8_t sda;         /* Sampled SDA line level, 0 or 1. */
	uint8_t confused;    /* Nonzero when the EasyMCP2221 compatibility heuristic
	                     * considers the I2C engine state inconsistent. */
	uint8_t initialized; /* Nonzero when the compatibility heuristic considers
	                     * the I2C engine initialized. */
} mcp2221_i2c_status_t;

/* I2C transfer kind.
 *
 * These replace the previous numeric magic values:
 *   0 = normal transfer
 *   1 = repeated-start transfer
 *   2 = write without stop condition
 */
typedef enum {
	MCP2221_I2C_KIND_NORMAL = 0,
	MCP2221_I2C_KIND_REPEATED_START = 1,
	MCP2221_I2C_KIND_NO_STOP = 2
} mcp2221_i2c_kind_t;

/* Opens MCP2221 device.
 * vid/pid: USB vendor/product (0x04D8,0x00DD default)
 * devnum: Device index if multiple found. (Default is first device: 0 )
 * usbserial: Device's USB serial to open. (Default NULL = ignore)
 *
 * Returns: allocated mcp2221_t handle, or NULL on error.
 */
mcp2221_t *mcp2221_open(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int usb_read_timeout_ms,
					  int cmd_retries, int debug_messages, int trace_packets);

// Function with optional scan_serial (in case USB serial is not enumerated).
mcp2221_t *mcp2221_open_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int usb_read_timeout_ms,
						   int cmd_retries, int debug_messages, int trace_packets, int scan_serial);

// Wrapper as used in Python
mcp2221_t *mcp2221_open_simple(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int i2c_speed_hz);
mcp2221_t *mcp2221_open_simple_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int i2c_speed_hz,
								  int scan_serial);

// Closes device
void mcp2221_close(mcp2221_t *dev);

/* Write raw USB command to device.
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on failure.
 */
mcp2221_error_code_t mcp2221_send_cmd(mcp2221_t *dev, const uint8_t *buf, size_t len, uint8_t *response /* 64-Byte Buffer */);

/* Set I2C clock speed in Hz. Preferred verb-based name. */
mcp2221_error_code_t mcp2221_i2c_set_speed(mcp2221_t *dev, uint32_t i2c_speed_hz);

/* Write data to I2C.
 *
 * addr: 7-bit I2C base address
 * kind:
 *   MCP2221_I2C_KIND_NORMAL         = normal write
 *   MCP2221_I2C_KIND_REPEATED_START = write with repeated start
 *   MCP2221_I2C_KIND_NO_STOP        = write without stop condition
 *
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on failure.
 */
/* Explicit-timeout variant.
 * i2c_timeout_ms controls the transfer watchdog for this operation.
 */
mcp2221_error_code_t mcp2221_i2c_write_ex(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len,
								 mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

/* Convenience variant.
 * Uses the device's configured USB read timeout as the I2C watchdog when it is
 * positive; otherwise it falls back to 20 ms.
 */
mcp2221_error_code_t mcp2221_i2c_write_simple(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len, mcp2221_i2c_kind_t kind);


/* Read data from I2C.
 *
 * addr: 7-bit I2C base address
 * kind:
 *   MCP2221_I2C_KIND_NORMAL         = normal read
 *   MCP2221_I2C_KIND_REPEATED_START = read with repeated start
 *
 * Returns MCP2221_ERR_OK on success or another mcp2221_error_code_t value on failure.
 */
/* Explicit-timeout variant.
 * i2c_timeout_ms controls the transfer watchdog for this operation.
 */
mcp2221_error_code_t mcp2221_i2c_read_ex(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len,
								mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

/* Convenience variant.
 * Uses the device's configured USB read timeout as the I2C watchdog when it is
 * positive; otherwise it falls back to 20 ms.
 */
mcp2221_error_code_t mcp2221_i2c_read_simple(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len, mcp2221_i2c_kind_t kind);

mcp2221_error_code_t mcp2221_i2c_status(mcp2221_t *dev, mcp2221_i2c_status_t *st);

// Release I2C (corresponds to _i2c_release)
mcp2221_error_code_t mcp2221_i2c_release(mcp2221_t *dev);

#endif	// MCP2221_H
