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

	for (size_t i = 0; i + 1 < in_len;) {
		uint16_t first = (uint16_t)(in[i] | (in[i + 1] << 8));
		uint32_t codepoint = first;
		i += 2;

		if (first == 0)
			break;

		if (first >= 0xD800u && first <= 0xDBFFu) {
			if (i + 1 < in_len) {
				uint16_t second = (uint16_t)(in[i] | (in[i + 1] << 8));
				if (second >= 0xDC00u && second <= 0xDFFFu) {
					codepoint =
						0x10000u +
						(((uint32_t)first - 0xD800u) << 10) +
						((uint32_t)second - 0xDC00u);
					i += 2;
				} else {
					codepoint = 0xFFFDu;
				}
			} else {
				codepoint = 0xFFFDu;
			}
		} else if (first >= 0xDC00u && first <= 0xDFFFu) {
			codepoint = 0xFFFDu;
		}

		if (codepoint < 0x80u && o + 1 < out_len) {
			out[o++] = (char)codepoint;
		} else if (codepoint < 0x800u && o + 2 < out_len) {
			out[o++] = (char)(0xC0u | (codepoint >> 6));
			out[o++] = (char)(0x80u | (codepoint & 0x3Fu));
		} else if (codepoint < 0x10000u && o + 3 < out_len) {
			out[o++] = (char)(0xE0u | (codepoint >> 12));
			out[o++] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
			out[o++] = (char)(0x80u | (codepoint & 0x3Fu));
		} else if (codepoint <= 0x10FFFFu && o + 4 < out_len) {
			out[o++] = (char)(0xF0u | (codepoint >> 18));
			out[o++] = (char)(0x80u | ((codepoint >> 12) & 0x3Fu));
			out[o++] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
			out[o++] = (char)(0x80u | (codepoint & 0x3Fu));
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
