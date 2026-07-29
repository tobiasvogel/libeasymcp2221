## Legacy API guard

CI checks newly added C, header and Markdown lines for deprecated 1.x API names.
Compatibility declarations belong in `include/error_codes.h`,
`include/exceptions.h` or `include/mcp2221_deprecated.h`. An intentional
compatibility use elsewhere must be marked on that line with
`/* legacy-api-ok */`.
