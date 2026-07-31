#ifndef MCP2221_ERROR_CODES_H
#define MCP2221_ERROR_CODES_H

/* Error codes, modelled after exceptions.py from EasyMCP2221 (v1.8.4).
 *
 * New code should use the mcp2221_error_code_t type and MCP2221_ERR_*
 * constants. The older mcp_err_t and MCP_ERR_* names remain as 1.x
 * compatibility aliases.
 */
typedef enum {
	MCP2221_ERR_OK = 0,              /* No error */
	MCP2221_ERR_USB = -1,            /* USB error */
	MCP2221_ERR_TIMEOUT = -2,        /* TimeoutError: I2C transaction timed out */
	MCP2221_ERR_NOT_ACK = -3,        /* NotAckError: I2C slave device did not acknowledge */
	MCP2221_ERR_LOW_SCL = -4,        /* LowSCLError: SCL remains low */
	MCP2221_ERR_LOW_SDA = -5,        /* LowSDAError: SDA remains low */
	MCP2221_ERR_INVALID = -6,        /* Invalid answer */
	MCP2221_ERR_I2C = -7,            /* I2C error */
	MCP2221_ERR_FLASH_WRITE = -8,    /* Flash write error */
	MCP2221_ERR_FLASH_PASSWD = -9,   /* Flash password error */
	MCP2221_ERR_GPIO_MODE = -10,     /* GPIO error */
	MCP2221_ERR_GENERIC = -11,       /* any other error */
	MCP2221_ERR_I2C_SHORT_READ = -12,/* I2C read completed before requested length */
	MCP2221_ERR_FLASH_READ = -13     /* Flash read error */
} mcp2221_error_code_t;

/* Legacy error-code macros remain temporarily for the next v2 cleanup step. */
#define MCP_ERR_OK MCP2221_ERR_OK
#define MCP_ERR_USB MCP2221_ERR_USB
#define MCP_ERR_TIMEOUT MCP2221_ERR_TIMEOUT
#define MCP_ERR_NOT_ACK MCP2221_ERR_NOT_ACK
#define MCP_ERR_LOW_SCL MCP2221_ERR_LOW_SCL
#define MCP_ERR_LOW_SDA MCP2221_ERR_LOW_SDA
#define MCP_ERR_INVALID MCP2221_ERR_INVALID
#define MCP_ERR_I2C MCP2221_ERR_I2C
#define MCP_ERR_FLASH_WRITE MCP2221_ERR_FLASH_WRITE
#define MCP_ERR_FLASH_PASSWD MCP2221_ERR_FLASH_PASSWD
#define MCP_ERR_GPIO_MODE MCP2221_ERR_GPIO_MODE
#define MCP_ERR_GENERIC MCP2221_ERR_GENERIC
#define MCP_ERR_I2C_SHORT_READ MCP2221_ERR_I2C_SHORT_READ
#define MCP_ERR_FLASH_READ MCP2221_ERR_FLASH_READ

#endif // MCP2221_ERROR_CODES_H
