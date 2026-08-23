#ifndef LIBEASYMCP2221_MCP2221_INTERNAL_STRING_H
#define LIBEASYMCP2221_MCP2221_INTERNAL_STRING_H

static inline int mcp2221_internal_ascii_case_equal(
	const char *lhs,
	const char *rhs) {
	while (*lhs != '\0' && *rhs != '\0') {
		unsigned char lhs_ch = (unsigned char)*lhs++;
		unsigned char rhs_ch = (unsigned char)*rhs++;

		if (lhs_ch >= 'A' && lhs_ch <= 'Z')
			lhs_ch = (unsigned char)(lhs_ch + ('a' - 'A'));
		if (rhs_ch >= 'A' && rhs_ch <= 'Z')
			rhs_ch = (unsigned char)(rhs_ch + ('a' - 'A'));

		if (lhs_ch != rhs_ch)
			return 0;
	}

	return *lhs == *rhs;
}

#endif /* LIBEASYMCP2221_MCP2221_INTERNAL_STRING_H */
