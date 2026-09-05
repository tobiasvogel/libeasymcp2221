/**
 * @file mcp2221_clock.h
 * @brief High-level MCP2221 clock-output configuration.
 */

#ifndef MCP2221_CLOCK_H
#define MCP2221_CLOCK_H

#include <stdint.h>
#include "mcp2221.h"

MCP2221_BEGIN_DECLS

/**
 * @brief Configure the MCP2221 clock output.
 *
 * Convenience wrapper matching EasyMCP2221 clock_config().
 * Valid duty cycles are 0, 25, 50, and 75 percent.
 * Valid frequencies are 375000, 750000, 1500000, 3000000, 6000000,
 * 12000000, and 24000000 Hz.
 *
 * GP1 must separately be assigned to CLK_OUT for the signal to appear.
 *
 * @param[in] dev Open MCP2221 device handle.
 * @param[in] duty_percent Duty cycle in percent.
 * @param[in] frequency_hz Output frequency in hertz.
 *
 * @return MCP2221_ERR_OK on success, MCP2221_ERR_INVALID for invalid
 *         arguments, or another mcp2221_error_code_t from
 *         mcp2221_sram_config().
 */
MCP2221_API mcp2221_error_code_t mcp2221_clock_config(
    mcp2221_t *dev, unsigned duty_percent, uint32_t frequency_hz);

MCP2221_END_DECLS

#endif /* MCP2221_CLOCK_H */
