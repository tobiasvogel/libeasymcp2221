#ifndef MCP2221_H
#define MCP2221_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "error_codes.h"
#include "mcp2221_errors.h"
#include "mcp2221_deprecated.h"

/* Preferred opaque device type. */
typedef struct mcp2221_device mcp2221_t;

/* Preferred opaque high-level I2C slave type. */
typedef struct mcp2221_i2c_slave mcp2221_i2c_slave_t;

/* 1.x source-compatibility aliases. */
typedef mcp2221_t MCP2221;
typedef mcp2221_i2c_slave_t I2C_Slave;

// For Debugging
typedef struct {
	uint16_t rlen;
	uint16_t txlen;
	uint8_t div;
	uint8_t ack;
	uint8_t st;
	uint8_t scl;
	uint8_t sda;
	uint8_t confused;
	uint8_t initialized;
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
 * Returns: pointer to MCP2221 (malloc) or NULL on error.
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

/* Preferred name for creating a high-level I2C slave helper. */
mcp2221_error_code_t mcp2221_i2c_slave_create(mcp2221_t *dev, mcp2221_i2c_slave_t *slave, uint8_t addr, int force,
								   uint32_t i2c_speed_hz, int reg_bytes, const char *reg_byteorder);

/* Legacy alias for mcp2221_i2c_slave_create(); scheduled for removal in a future major version. */
MCP2221_DEPRECATED("use mcp2221_i2c_slave_create") mcp_err_t mcp2221_create_i2c_slave(MCP2221 *dev, I2C_Slave *slave, uint8_t addr, int force, uint32_t i2c_speed_hz,
									   int reg_bytes, const char *reg_byteorder);

/* Write raw USB command to device.
 * Returns 0 on success or error code
 */
mcp2221_error_code_t mcp2221_send_cmd(mcp2221_t *dev, const uint8_t *buf, size_t len, uint8_t *response /* 64-Byte Buffer */);

/* Set I2C clock speed in Hz. Preferred verb-based name. */
mcp2221_error_code_t mcp2221_i2c_set_speed(mcp2221_t *dev, uint32_t i2c_speed_hz);

/* Legacy alias for mcp2221_i2c_set_speed(); scheduled for removal in a future major version. */
MCP2221_DEPRECATED("use mcp2221_i2c_set_speed") mcp_err_t mcp2221_i2c_speed(MCP2221 *dev, uint32_t i2c_speed_hz);

/* Write data to I2C.
 *
 * addr: 7-bit I2C base address
 * kind:
 *   MCP2221_I2C_KIND_NORMAL         = normal write
 *   MCP2221_I2C_KIND_REPEATED_START = write with repeated start
 *   MCP2221_I2C_KIND_NO_STOP        = write without stop condition
 *
 * Returns 0 on success or error code.
 */
mcp2221_error_code_t mcp2221_i2c_write_ex(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len,
								 mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

/* Legacy alias for mcp2221_i2c_write_ex(); scheduled for removal in a future major version. */
MCP2221_DEPRECATED("use mcp2221_i2c_write_ex") mcp_err_t mcp2221_i2c_write(MCP2221 *dev, uint8_t addr, const uint8_t *data, size_t len, mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

mcp2221_error_code_t mcp2221_i2c_write_simple(mcp2221_t *dev, uint8_t addr, const uint8_t *data, size_t len, mcp2221_i2c_kind_t kind);


/* Read data from I2C.
 *
 * addr: 7-bit I2C base address
 * kind:
 *   MCP2221_I2C_KIND_NORMAL         = normal read
 *   MCP2221_I2C_KIND_REPEATED_START = read with repeated start
 *
 * Returns 0 on success or error code.
 */
mcp2221_error_code_t mcp2221_i2c_read_ex(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len,
								mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

/* Legacy alias for mcp2221_i2c_read_ex(); scheduled for removal in a future major version. */
MCP2221_DEPRECATED("use mcp2221_i2c_read_ex") mcp_err_t mcp2221_i2c_read(MCP2221 *dev, uint8_t addr, uint8_t *data, size_t len, mcp2221_i2c_kind_t kind, int i2c_timeout_ms);

mcp2221_error_code_t mcp2221_i2c_read_simple(mcp2221_t *dev, uint8_t addr, uint8_t *data, size_t len, mcp2221_i2c_kind_t kind);

mcp2221_error_code_t mcp2221_i2c_status(mcp2221_t *dev, mcp2221_i2c_status_t *st);

// Release I2C (corresponds to _i2c_release)
mcp2221_error_code_t mcp2221_i2c_release(mcp2221_t *dev);

#endif	// MCP2221_H
