/**
 * @file mcp2221_error_codes.h
 * @brief Error codes returned by the libeasymcp2221 v2 API.
 */

#ifndef MCP2221_ERROR_CODES_H
#define MCP2221_ERROR_CODES_H

/**
 * @brief Error codes used by libeasymcp2221.
 *
 * Most public functions return MCP2221_ERR_OK on success or one of the
 * negative values below on failure. The error model is based on the
 * EasyMCP2221 exception hierarchy while providing explicit C return values.
 *
 * @see mcp2221_error_code_to_string()
 */
typedef enum {
	MCP2221_ERR_OK = 0,               /**< No error. */
	MCP2221_ERR_USB = -1,             /**< Generic USB transport error. */
	MCP2221_ERR_TIMEOUT = -2,         /**< I2C or USB operation timed out. */
	MCP2221_ERR_NOT_ACK = -3,         /**< I2C target did not acknowledge. */
	MCP2221_ERR_LOW_SCL = -4,         /**< SCL remained low. */
	MCP2221_ERR_LOW_SDA = -5,         /**< SDA remained low. */
	MCP2221_ERR_INVALID = -6,         /**< Invalid argument, value, state, or device response. */
	MCP2221_ERR_I2C = -7,             /**< Generic I2C error. */
	MCP2221_ERR_FLASH_WRITE = -8,     /**< Flash write failed. */
	MCP2221_ERR_FLASH_PASSWD = -9,    /**< Flash password error. */
	MCP2221_ERR_GPIO_MODE = -10,      /**< Invalid or unsupported GPIO mode. */
	MCP2221_ERR_GENERIC = -11,        /**< Unclassified error. */
	MCP2221_ERR_I2C_SHORT_READ = -12, /**< I2C read completed before the requested length. */
	MCP2221_ERR_FLASH_READ = -13,     /**< Flash read failed. */
	MCP2221_ERR_NOT_FOUND = -14,      /**< Requested MCP2221 device was not found. */
	MCP2221_ERR_NO_MEMORY = -15,      /**< Memory allocation failed. */
	MCP2221_ERR_ACCESS = -16,         /**< Access or permission was denied. */
	MCP2221_ERR_BUSY = -17,           /**< Device or USB interface is busy. */
	MCP2221_ERR_USB_INIT = -18,       /**< USB backend initialization failed. */
	MCP2221_ERR_USB_ENUM = -19,       /**< USB device enumeration failed. */
	MCP2221_ERR_USB_OPEN = -20,       /**< USB device open failed. */
	MCP2221_ERR_USB_CLAIM = -21,      /**< USB interface claim failed. */
	MCP2221_ERR_COMMAND_FAILED = -22, /**< MCP2221 rejected or failed a command. */
	MCP2221_ERR_PROTOCOL = -23        /**< Invalid or mismatched MCP2221 protocol response. */
} mcp2221_error_code_t;

#endif // MCP2221_ERROR_CODES_H
