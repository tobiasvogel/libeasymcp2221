#ifndef MCP2221_ERRORS_H
#define MCP2221_ERRORS_H
#include "mcp2221_export.h"

#include "mcp2221_error_codes.h"

MCP2221_BEGIN_DECLS

/* Convert an error code to its stable symbolic name. */
MCP2221_API const char *mcp2221_error_code_to_string(mcp2221_error_code_t code);

MCP2221_END_DECLS
#endif // MCP2221_ERRORS_H
