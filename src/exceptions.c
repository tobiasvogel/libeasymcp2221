#include "exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *mcp_error_code_to_string(mcp_err_t code) {
	switch (code) {
		case MCP_ERR_OK:
			return "OK";
		case MCP_ERR_USB:
			return "USBError";
		case MCP_ERR_NOT_ACK:
			return "NotAckError";
		case MCP_ERR_TIMEOUT:
			return "TimeoutError";
		case MCP_ERR_LOW_SCL:
			return "LowSCLError";
		case MCP_ERR_LOW_SDA:
			return "LowSDAError";
		case MCP_ERR_INVALID:
			return "InvalidAnswerError";
		case MCP_ERR_I2C:
			return "GenericI2CError";
		case MCP_ERR_GENERIC:
		default:
			return "GenericError";
	}
}

mcp_error_t *mcp_error_init(mcp_error_t *err, mcp_err_t code) {
	if (err == NULL)
		return NULL;
	err->code = code;
	err->message = NULL;
	return err;
}

int mcp_error_set_message(mcp_error_t *err, const char *message) {
	if (err == NULL)
		return -1;
	/* free previous */
	if (err->message) {
		free(err->message);
		err->message = NULL;
	}
	if (message == NULL) {
		return 0;
	}
	size_t len = strlen(message);
	char *dup = (char *)malloc(len + 1);
	if (!dup)
		return -1;
	memcpy(dup, message, len + 1);
	err->message = dup;
	return 0;
}

void mcp_error_clear(mcp_error_t *err) {
	if (!err)
		return;
	if (err->message) {
		free(err->message);
		err->message = NULL;
	}
}

char *mcp_error_to_string_dup(const mcp_error_t *err) {
	if (!err)
		return NULL;
	const char *code_str = mcp_error_code_to_string(err->code);
	if (err->message == NULL) {
		/* Code only */
		size_t n = strlen(code_str);
		char *out = (char *)malloc(n + 1);
		if (!out)
			return NULL;
		memcpy(out, code_str, n + 1);
		return out;
	} else {
		/* "Code: message" */
		size_t n1 = strlen(code_str);
		size_t n2 = strlen(err->message);
		/* +2 => ": " and terminating NUL */
		char *out = (char *)malloc(n1 + 2 + n2 + 1);
		if (!out)
			return NULL;
		memcpy(out, code_str, n1);
		out[n1] = ':';
		out[n1 + 1] = ' ';
		memcpy(out + n1 + 2, err->message, n2 + 1);
		return out;
	}
}
