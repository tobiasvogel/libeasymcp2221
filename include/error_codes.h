#ifndef MCP2221_ERROR_CODES_H
#define MCP2221_ERROR_CODES_H

// Error-codes, as in exceptions.py from EasyMCP2221 (v1.8.4)
typedef enum {
	MCP_ERR_OK = 0,			   /* No error */
	MCP_ERR_USB = -1,		   /* USB error */
	MCP_ERR_TIMEOUT = -2,	   /* TimeoutError: I2C transaction timed out */
	MCP_ERR_NOT_ACK = -3,	   /* NotAckError: I2C slave device did not acknowledge */
	MCP_ERR_LOW_SCL = -4,	   /* LowSCLError: SCL remains low */
	MCP_ERR_LOW_SDA = -5,	   /* LowSDAError: SDA remains low */
	MCP_ERR_INVALID = -6,	   /* Invalid answer */
	MCP_ERR_I2C = -7,		   /* I2C error */
	MCP_ERR_FLASH_WRITE = -8,     /* Flash write error */
	MCP_ERR_FLASH_PASSWD = -9,    /* Flash password error */
	MCP_ERR_GPIO_MODE = -10,      /* GPIO error */
	MCP_ERR_GENERIC = -11,        /* any other error */
	MCP_ERR_I2C_SHORT_READ = -12, /* I2C read completed before requested length */
	MCP_ERR_FLASH_READ = -13      /* Flash read error */
} mcp_err_t;

#endif // MCP2221_ERROR_CODES_H
