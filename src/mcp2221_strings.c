#include "mcp2221_internal.h"

#include <stddef.h>
#include <string.h>

#include "mcp2221_internal_constants.h"

void mcp2221_internal_utf16le_to_utf8(const uint8_t *in, size_t in_len, char *out, size_t out_len) {
	if (!out || out_len == 0)
		return;

	size_t o = 0;
	if (!in) {
		out[o] = '\0';
		return;
	}

	for (size_t i = 0; i + 1 < in_len && o + 1 < out_len; i += 2) {
		uint16_t code = (uint16_t)(in[i] | (in[i + 1] << 8));
		if (code == 0)
			break;
		if (code < 0x80) {
			out[o++] = (char)code;
		} else if (code < 0x800 && o + 2 < out_len) {
			out[o++] = (char)(0xC0 | (code >> 6));
			out[o++] = (char)(0x80 | (code & 0x3F));
		} else if (o + 3 < out_len) {
			out[o++] = (char)(0xE0 | (code >> 12));
			out[o++] = (char)(0x80 | ((code >> 6) & 0x3F));
			out[o++] = (char)(0x80 | (code & 0x3F));
		} else {
			break;
		}
	}
	out[o] = '\0';
}

void mcp2221_internal_parse_wchar_structure(
	const uint8_t *buf, size_t buf_len, uint8_t structure_length,
	char *out, size_t out_len) {
	if (!out || out_len == 0)
		return;
	if (!buf) {
		out[0] = '\0';
		return;
	}

	size_t declared = structure_length >= MCP2221_USB_STRING_DESCRIPTOR_HEADER_SIZE
		? (size_t)(structure_length - MCP2221_USB_STRING_DESCRIPTOR_HEADER_SIZE)
		: 0;
	if (declared > buf_len)
		declared = buf_len;
	mcp2221_internal_utf16le_to_utf8(buf, declared, out, out_len);
}

void mcp2221_internal_parse_factory_serial(
	const uint8_t *buf, size_t buf_len, uint8_t structure_length,
	char *out, size_t out_len) {
	if (!out || out_len == 0)
		return;
	if (!buf) {
		out[0] = '\0';
		return;
	}

	size_t declared = structure_length;
	if (declared > buf_len)
		declared = buf_len;
	if (declared >= out_len)
		declared = out_len - 1;
	memcpy(out, buf, declared);
	out[declared] = '\0';
}
