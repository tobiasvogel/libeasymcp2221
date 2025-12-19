#ifndef MCP2221_H
#define MCP2221_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "error_codes.h"
#include "exceptions.h"

// Opaque device handle struct
typedef struct MCP2221 MCP2221;
typedef struct I2C_Slave I2C_Slave;

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

/* Opens MCP2221 device.
 * vid/pid: USB vendor/product (0x04D8,0x00DD default)
 * devnum: Device index if multiple found. (Default is first device: 0 )
 * usbserial: Device's USB serial to open. (Default NULL = ignore)
 *
 * Returns: pointer to MCP2221 (malloc) or NULL on error.
 */
MCP2221 *mcp2221_open(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int read_timeout_ms,
					  int cmd_retries, int debug_messages, int trace_packets);

// Variante mit optionalem scan_serial (falls USB-Seriennummer nicht enumeriert wird).
MCP2221 *mcp2221_open_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int read_timeout_ms,
						   int cmd_retries, int debug_messages, int trace_packets, int scan_serial);

// Wrapper as used in Python
MCP2221 *mcp2221_open_simple(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int clock_hz); 
MCP2221 *mcp2221_open_simple_scan(uint16_t vid, uint16_t pid, int devnum, const char *usbserial, int clock_hz,
								  int scan_serial);

// Closes device
void mcp2221_close(MCP2221 *dev);

mcp_err_t mcp2221_create_i2c_slave(MCP2221 *dev, I2C_Slave *slave, uint8_t addr, int force, uint32_t speed_hz,
									   int reg_bytes, const char *reg_byteorder);

/* Write raw USB command to device.
 * Returns 0 on success or error code
 */
mcp_err_t mcp2221_send_cmd(MCP2221 *dev, const uint8_t *buf, size_t len, uint8_t *response /* 64-Byte Buffer */);

// Set I2C clock speed (Hz). Wrapper over the low-level USB backend.
mcp_err_t mcp2221_i2c_speed(MCP2221 *dev, uint32_t speed_hz);

/* Write data to I2C: addr = 7-bit I2C base address
 * nonstop = 1, 0 = normal write
 *
 * Returns 0 on success or error code.
 */
mcp_err_t mcp2221_i2c_write(MCP2221 *dev, uint8_t addr, const uint8_t *data, size_t len, int kind, int timeout_ms);

mcp_err_t mcp2221_i2c_write_simple(MCP2221 *dev, uint8_t addr, const uint8_t *data, size_t len, int kind);


/* Read data from I2C: addr = 7-bit address
 * restart = 1, 0 = normal read
 *
 * Returns 0 on success or error code.
 */
mcp_err_t mcp2221_i2c_read(MCP2221 *dev, uint8_t addr, uint8_t *data, size_t len, int kind, int timeout_ms);

mcp_err_t mcp2221_i2c_read_simple(MCP2221 *dev, uint8_t addr, uint8_t *data, size_t len, int kind);

mcp_err_t mcp2221_i2c_status(MCP2221 *dev, mcp2221_i2c_status_t *st);

// Release I2C (corresponds to _i2c_release)
mcp_err_t mcp2221_i2c_release(MCP2221 *dev);

#endif	// MCP2221_H
