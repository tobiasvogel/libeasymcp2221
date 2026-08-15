/**
 * @file mcp2221_errors.h
 * @brief Error-code formatting helpers.
 */

#ifndef MCP2221_ERRORS_H
#define MCP2221_ERRORS_H
#include "mcp2221_export.h"

#include "mcp2221_error_codes.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Convert an error code to its stable symbolic name.
 *
 * The returned string has static storage duration and must not be freed.
 * Known error codes map to names such as `"OK"`, `"USBError"`, and
 * `"InvalidError"`. Unknown values map to `"GenericError"`.
 *
 * @param[in] code Error code to format.
 *
 * @return Pointer to a null-terminated static string.
 */
MCP2221_API const char *mcp2221_error_code_to_string(mcp2221_error_code_t code);

MCP2221_END_DECLS
#endif // MCP2221_ERRORS_H
