/**
 * @file libeasymcp2221.h
 * @brief Convenience header for the complete supported public v2 API.
 *
 * Applications may include this header when they prefer access to all
 * libeasymcp2221 public modules through a single include. Individual
 * `mcp2221_*.h` headers may instead be included directly when a narrower
 * dependency surface is desired.
 */

#ifndef LIBEASYMCP2221_H
#define LIBEASYMCP2221_H

#include "mcp2221.h"
#include "mcp2221_constants.h"
#include "mcp2221_gpio.h"
#include "mcp2221_gpio_poll.h"
#include "mcp2221_pin.h"
#include "mcp2221_sram.h"
#include "mcp2221_clock.h"
#include "mcp2221_flash.h"
#include "mcp2221_flash_info.h"
#include "mcp2221_flash_settings.h"
#include "mcp2221_analog.h"
#include "mcp2221_i2c_slave.h"
#include "mcp2221_smbus.h"
#include "mcp2221_usb.h"
#include "mcp2221_errors.h"

#endif /* LIBEASYMCP2221_H */