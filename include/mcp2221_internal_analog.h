#ifndef MCP2221_INTERNAL_ANALOG_H
#define MCP2221_INTERNAL_ANALOG_H

#include "mcp2221_internal.h"

MCP2221_BEGIN_DECLS

mcp2221_error_code_t mcp2221_internal_analog_set_vdd(mcp2221_t *dev, double volts);
mcp2221_error_code_t mcp2221_internal_analog_get_vdd(const mcp2221_t *dev, double *volts);
mcp2221_error_code_t mcp2221_internal_get_analog_reference_voltage(
	const mcp2221_t *dev,
	mcp2221_analog_voltage_reference_t reference,
	double *volts);

MCP2221_END_DECLS

#endif /* MCP2221_INTERNAL_ANALOG_H */
