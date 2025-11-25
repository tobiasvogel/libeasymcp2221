#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>
#include "error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Optional container with error-message. For error-cody only, use mcp_err_t.
typedef struct {
	mcp_err_t code;
	char *message; /* NULL if message empty */
} mcp_error_t;

// Returns a string-representation of the error code
const char *mcp_error_code_to_string(mcp_err_t code);

// initializes mcp_error_t (code and message=NULL).
mcp_error_t *mcp_error_init(mcp_error_t *err, mcp_err_t code);

// Overwrites (or sets) the message. Return value: 0 = ok, -1 = malloc failed.
int mcp_error_set_message(mcp_error_t *err, const char *message);

// Frees up mcp_error_t allocated resources
void mcp_error_clear(mcp_error_t *err);

// Error String: returns "CODE: message"
char *mcp_error_to_string_dup(const mcp_error_t *err);

#ifdef __cplusplus
}
#endif

#endif	// EXCEPTIONS_H
