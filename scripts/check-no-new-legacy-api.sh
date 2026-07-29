#!/bin/sh
set -eu

base_ref=${1:-}

if [ -z "$base_ref" ]; then
	echo "usage: $0 <base-ref>" >&2
	exit 2
fi

if ! git rev-parse --verify "$base_ref^{commit}" >/dev/null 2>&1; then
	echo "legacy API check: base ref '$base_ref' does not exist" >&2
	exit 2
fi

merge_base=$(git merge-base "$base_ref" HEAD)
tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT HUP INT TERM

git diff --unified=0 --no-color "$merge_base"...HEAD -- \
	'*.c' '*.h' '*.md' \
	':(exclude)include/error_codes.h' \
	':(exclude)include/exceptions.h' \
	':(exclude)include/mcp2221_deprecated.h' \
	> "$tmp"

awk '
BEGIN {
	path = ""
	failed = 0
	pattern = "(MCP_ERR_[A-Z0-9_]+|mcp_err_t|(^|[^[:alnum:]_])MCP2221[[:space:]]*\\*|(^|[^[:alnum:]_])I2C_Slave([^[:alnum:]_]|$)|mcp_error_code_to_string[[:space:]]*\\(|(^|[^[:alnum:]_])(i2c_slave|smbus)_[[:alnum:]_]+[[:space:]]*\\(|mcp2221_i2c_speed[[:space:]]*\\(|mcp2221_set_pin_(function|functions)[[:space:]]*\\(|MCP_CONFIG_KEEP([^[:alnum:]_]|$)|DEV_DEFAULT_(VID|PID)([^[:alnum:]_]|$)|(^|[^[:alnum:]_])PACKET_SIZE([^[:alnum:]_]|$)|(^|[^[:alnum:]_])I2C_ADDR_7BIT_MAX([^[:alnum:]_]|$)|(^|[^[:alnum:]_])I2C_SMBUS_BLOCK_MAX([^[:alnum:]_]|$))"
}
/^\+\+\+ b\// {
	path = substr($0, 7)
	next
}
/^\+/ && !/^\+\+\+/ {
	line = substr($0, 2)
	if (line ~ pattern && line !~ /legacy-api-ok/) {
		printf "%s: new legacy API use: %s\n", path, line > "/dev/stderr"
		failed = 1
	}
}
END {
	if (failed) {
		print "Use the preferred mcp2221_* API names. For an intentional compatibility declaration, append: /* legacy-api-ok */" > "/dev/stderr"
		exit 1
	}
}
' "$tmp"

echo "legacy API check passed"
