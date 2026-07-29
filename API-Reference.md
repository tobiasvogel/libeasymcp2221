# EasyMCP2221 v1.8.4 → C Port API Reference

This document maps common EasyMCP2221 Python concepts to the preferred libeasymcp2221 C API names. The C library uses explicit handles, output buffers and `mcp_err_t` return values instead of Python objects and exceptions.

| Python (EasyMCP2221 v1.8.4) | Preferred libeasymcp2221 C API | Notes |
|---|---|---|
| `Device(...)` (open, reuse, scan_serial) | `mcp2221_open_scan(vid,pid,devnum,usbserial,usb_read_timeout_ms,retries,debug,trace,scan_serial)` / `mcp2221_open_simple(vid,pid,devnum,usbserial,i2c_speed_hz)` | Catalog/handle reuse; optional flash serial scan. |
| `Device.close()` | `mcp2221_close()` | Releases the MCP2221 handle and any internally detached kernel driver. |
| `Device.send_cmd(buf)` | `mcp2221_send_cmd()` | Same retry/timeout concept; transfers fixed-size USB reports internally. |
| `Device._i2c_status()` | `mcp2221_i2c_status()` | Same `confused`/`initialized` heuristics. |
| `Device._i2c_release()` | `mcp2221_i2c_release()` | Cancels or releases a stuck I2C transaction. |
| `Device.I2C_speed(speed)` | `mcp2221_i2c_set_speed(i2c_speed_hz)` | Python `round()` ties-to-even behavior. Legacy alias: `mcp2221_i2c_speed()`. |
| `Device.I2C_write(addr,data,kind,timeout_ms)` | `mcp2221_i2c_write_ex(addr,data,len,kind,i2c_timeout_ms)` | Same chunking/state machine. Legacy alias: `mcp2221_i2c_write()`. |
| `Device.I2C_read(addr,size,kind,timeout_ms)` | `mcp2221_i2c_read_ex(addr,data,len,kind,i2c_timeout_ms)` | Returns `MCP_ERR_I2C_SHORT_READ` if the device reports success before all requested bytes were received. Legacy alias: `mcp2221_i2c_read()`. |
| `Device.GPIO_write(gp0..gp3)` | `mcp2221_gpio_write()` | `MCP2221_GPIO_KEEP` preserves a pin value. |
| `Device.GPIO_read()` | `mcp2221_gpio_read()` / `mcp2221_gpio_read_mask()` | Mask variant mirrors Python `None` for non-GPIO pins. |
| `Device.GPIO_poll()` | `mcp2221_gpio_poll_events()` / `mcp2221_gpio_poll()` | Event list (rise/fall, time/last_time) plus simple change API. |
| `Device.set_pin_function(gp0..gp3, out0..out3)` | `mcp2221_pin_set_functions()` / `mcp2221_pin_set_function()` | Batch like Python; `out` only valid with GPIO output. Legacy aliases: `mcp2221_set_pin_*()`. |
| `Device.SRAM_config(...)` | `mcp2221_sram_config()` | Same USB semantics; preserves GPIO output bits through the internal GPIO cache. |
| `Device.ADC_config(ref,vdd)` | `mcp2221_adc_config(ref_str)` | `vdd` is not supported. |
| `Device.ADC_read(norm,volts)` | `mcp2221_adc_read_raw(out[3])` | Raw 10-bit values; no normalized/voltage convenience layer. |
| `Device.DAC_config(ref,out,vdd)` | `mcp2221_dac_config_out(ref_str,out_code)` / `mcp2221_dac_config(ref_str)` | Reference plus optional raw output value (0..31); no `vdd`. |
| `Device.DAC_write(out,norm,volts)` | `mcp2221_dac_write_raw(code)` | Raw 5-bit value only. |
| `Device.clock_config(duty,freq)` | `mcp2221_clock_config(duty_percent,freq_str)` | Same allowed values. |
| `Device.IOC_read()` | `mcp2221_ioc_read()` | Same flag behavior. |
| `IOC_clear()` | `mcp2221_ioc_clear()` | Same. |
| `IOC_config(edge)` | `mcp2221_ioc_config(edge_str)` | `none`, `rising`, `falling`, `both`. |
| `Device.read_flash_info()` + parser | `mcp2221_flash_read_info()` | Reads all flash sections and converts UTF-16LE strings to UTF-8. Returns `MCP_ERR_FLASH_READ` on read failures. |
| `save_config()` | `mcp2221_flash_save_config()` | SRAM-to-flash mapping including GPIO cache; reports flash read and write errors separately. |
| `I2C_Slave.I2C_Slave` | `mcp2221_i2c_slave_create()` + `mcp2221_i2c_slave_*()` | Same no-stop write plus repeated-start read logic. Legacy aliases: `i2c_slave_*()`. |
| `smbus.SMBus` (subset) | `mcp2221_smbus_init()` / `mcp2221_smbus_close()` / `mcp2221_smbus_*()` | Closes internally-owned MCP2221 handles only. Legacy aliases: `smbus_*()`. |

## C API naming scheme

Preferred public names use:

- functions: `mcp2221_<domain>_<verb>[_object]()`
- types: `mcp2221_<domain>_<name>_t`
- enum constants and public macros: `MCP2221_<DOMAIN>_<NAME>`

Older names such as `i2c_slave_*()`, `smbus_*()`, `mcp2221_i2c_speed()`, `mcp2221_set_pin_function()` and `MCP_CONFIG_KEEP` remain available as deprecated compatibility aliases throughout the 1.x series. They may be removed in the next major version, currently planned as 2.0. Define `MCP2221_NO_DEPRECATED_WARNINGS` before including public headers to silence compatibility warnings temporarily during migration. New code, examples and documentation should use only the preferred names.

## I2C transfer kinds

Use `mcp2221_i2c_kind_t`:

| Kind | Meaning |
|---|---|
| `MCP2221_I2C_KIND_NORMAL` | Normal transfer with stop condition. |
| `MCP2221_I2C_KIND_REPEATED_START` | Repeated-start transfer. Valid for reads and writes. |
| `MCP2221_I2C_KIND_NO_STOP` | Write without stop condition. Valid for writes. |

## Error handling

Most functions return `MCP_ERR_OK` on success or another `mcp_err_t` value on failure. Functions returning payload sizes use `size_t *` output parameters for lengths.

Important error codes include:

| Error code | Typical meaning |
|---|---|
| `MCP_ERR_USB` | USB/libusb operation failed. |
| `MCP_ERR_TIMEOUT` | USB or I2C operation timed out. |
| `MCP_ERR_NOT_ACK` | I2C address or data was not acknowledged. |
| `MCP_ERR_LOW_SCL` / `MCP_ERR_LOW_SDA` | I2C bus line is held low. |
| `MCP_ERR_INVALID` | Invalid argument or unsupported value. |
| `MCP_ERR_I2C` | Generic I2C state-machine failure. |
| `MCP_ERR_I2C_SHORT_READ` | I2C read completed before all requested bytes were received. |
| `MCP_ERR_FLASH_READ` | Flash read operation failed. |
| `MCP_ERR_FLASH_WRITE` | Flash write operation failed. |
| `MCP_ERR_FLASH_PASSWD` | Flash write password error. |
| `MCP_ERR_GPIO_MODE` | Pin is not configured for the requested GPIO operation. |

Use `mcp_error_code_to_string()` to convert error codes to stable symbolic names.

## Resource ownership

- `mcp2221_open*()` returns an owned `mcp2221_t *`; call `mcp2221_close()` when done.
- `mcp2221_smbus_init(bus, existing_mcp, ...)` borrows `existing_mcp` when it is non-NULL. `mcp2221_smbus_close()` does not close borrowed MCP2221 handles.
- If `mcp2221_smbus_init()` opens its own MCP2221 handle, `mcp2221_smbus_close()` releases it.

## Macro naming

Public macros use the `MCP2221_*` prefix. Unprefixed legacy macros remain available as compatibility aliases during the 1.x series and should not be used in new code.

Examples:

| Legacy macro | Preferred macro |
|---|---|
| `DEV_DEFAULT_VID` | `MCP2221_DEV_DEFAULT_VID` |
| `DEV_DEFAULT_PID` | `MCP2221_DEV_DEFAULT_PID` |
| `PACKET_SIZE` | `MCP2221_PACKET_SIZE` |
| `I2C_ADDR_7BIT_MAX` | `MCP2221_I2C_ADDR_7BIT_MAX` |
| `I2C_SMBUS_BLOCK_MAX` | `MCP2221_I2C_SMBUS_BLOCK_MAX` |
| `MCP_CONFIG_KEEP` | `MCP2221_CONFIG_KEEP` |
| `MCP_GPIO_POLL_MASK_RISE(pin)` | `MCP2221_GPIO_POLL_MASK_RISE(pin)` |

## Differences / Not Covered

- Python exceptions are represented as explicit `mcp_err_t` values.
- No high-level USB power-management helpers.
- No `vdd` handling in ADC/DAC config; no normalized/voltage convenience layer.
