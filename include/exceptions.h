#ifndef MCP2221_EXCEPTIONS_H
#define MCP2221_EXCEPTIONS_H

#include "mcp2221_deprecated.h"
#include "mcp2221_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compatibility header for the deprecated 1.x exception/message-wrapper API.
 * New code should include mcp2221.h or mcp2221_errors.h and use
 * mcp2221_error_code_t plus mcp2221_error_code_to_string().
 */

/* Legacy name kept for the 1.x series; prefer mcp2221_error_code_to_string(). */
MCP2221_DEPRECATED("use mcp2221_error_code_to_string") const char *mcp_error_code_to_string(mcp2221_error_code_t code);

/* Deprecated message-wrapper API. It is not used by the core library; keep it
 * only for source compatibility with early libeasymcp2221 versions.
 */
typedef struct {
	mcp2221_error_code_t code;
	char *message; /* NULL if message empty */
} mcp_error_t;

MCP2221_DEPRECATED("message wrapper API is deprecated; use mcp2221_error_code_t")
mcp_error_t *mcp_error_init(mcp_error_t *err, mcp2221_error_code_t code);

MCP2221_DEPRECATED("message wrapper API is deprecated; use mcp2221_error_code_t")
int mcp_error_set_message(mcp_error_t *err, const char *message);

MCP2221_DEPRECATED("message wrapper API is deprecated; use mcp2221_error_code_t")
void mcp_error_clear(mcp_error_t *err);

MCP2221_DEPRECATED("message wrapper API is deprecated; use mcp2221_error_code_t")
char *mcp_error_to_string_dup(const mcp_error_t *err);

#ifdef __cplusplus
}
#endif

#endif	// MCP2221_EXCEPTIONS_H
