#ifndef MCP2221_ERROR_CODES_H
#define MCP2221_ERROR_CODES_H

/* Error codes, modelled after exceptions.py from EasyMCP2221 (v1.8.4). */
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


#endif // MCP2221_ERROR_CODES_H
