# EasyMCP2221 v1.8.4 → C Port API Reference

| Python (EasyMCP2221 v1.8.4) | libeasymcp2221 (v1.1.0) | Notes |
|---|---|---|
| `Device(...)` (open, reuse, scan_serial) | `mcp2221_open_scan(vid,pid,devnum,usbserial,timeouts,retries,debug,trace,scan_serial)` / `mcp2221_open_simple(vid,pid,devnum,usbserial,clock_hz)` | Catalog/handle reuse; optional flash serial scan (Python-like). |
| `Device.send_cmd(buf)` | `mcp2221_send_cmd()` | Same retry/timeout logic. |
| `Device._i2c_status()` | `mcp2221_i2c_status()` | Same `confused/initialized` heuristics. |
| `Device._i2c_release()` | `mcp2221_i2c_release()` | Same semantics. |
| `Device.I2C_speed(speed)` | `mcp2221_i2c_speed(speed_hz)` | Python `round()` ties-to-even behavior. |
| `Device.I2C_write(addr,data,kind,timeout_ms)` | `mcp2221_i2c_write(addr,data,len,kind,timeout_ms)` | Same chunking/state machine. |
| `Device.I2C_read(addr,size,kind,timeout_ms)` | `mcp2221_i2c_read(addr,data,len,kind,timeout_ms)` | Same. |
| `Device.GPIO_write(gp0..gp3)` | `mcp2221_gpio_write()` | `-1` = preserve (Python `None`). |
| `Device.GPIO_read()` | `mcp2221_gpio_read()` / `mcp2221_gpio_read_mask()` | Mask variant mirrors Python `None` for non-GPIO. |
| `Device.GPIO_poll()` | `mcp2221_gpio_poll_events()` / `mcp2221_gpio_poll()` | Event list (RISE/FALL, time/last_time) + simple change API. |
| `Device.set_pin_function(gp0..gp3, out0..out3)` | `mcp2221_set_pin_functions()` / `mcp2221_set_pin_function()` | Batch like Python; `out` only valid with GPIO_OUT. |
| `Device.SRAM_config(...)` | `mcp2221_sram_config()` | Same USB semantics; preserves GPIO out bits via cache. |
| `Device.ADC_config(ref,vdd)` | `mcp2221_adc_config(ref_str)` | `vdd` not supported. |
| `Device.ADC_read(norm,volts)` | `mcp2221_adc_read_raw(out[3])` | Raw 10-bit (no norm/volts layer). |
| `Device.DAC_config(ref,out,vdd)` | `mcp2221_dac_config_out(ref_str,out_code)` / `mcp2221_dac_config(ref_str)` | Ref + optional raw out (0..31); no `vdd`. |
| `Device.DAC_write(out,norm,volts)` | `mcp2221_dac_write_raw(code)` | Raw 5-bit only. |
| `Device.clock_config(duty,freq)` | `mcp2221_clock_config(duty_percent,freq_str)` | Same allowed values. |
| `Device.IOC_read()` | `mcp2221_ioc_read()` | Same flag behavior. |
| `IOC_clear()` | `mcp2221_ioc_clear()` | Same. |
| `IOC_config(edge)` | `mcp2221_ioc_config(edge_str)` | `none/rising/falling/both`. |
| `Device.read_flash_info()` + Parser | `mcp2221_flash_read_info()` | Reads all flash sections, UTF-8 strings. |
| `save_config()` | `mcp2221_flash_save_config()` | SRAM→Flash mapping incl. GPIO cache. |
| `I2C_Slave.I2C_Slave` | `mcp2221_create_i2c_slave()` + `i2c_slave_*()` | Same nonstop write + restart read logic. |
| `smbus.SMBus` (subset) | `smbus_*()` | Core ops; constructor semantics differ. |

### Differences / Not Covered
- Exceptions → error codes (`mcp_err_t`).
- No high-level USB power-management helpers.
- No `vdd` handling in ADC/DAC config; no norm/volt convenience layer.
