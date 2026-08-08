# EasyMCP2221 v1.8.4 → libeasymcp2221 v2 API Reference

This document maps common EasyMCP2221 Python concepts to the
libeasymcp2221 v2 C API. The C library uses explicit handles, caller-provided
storage, output buffers and `mcp2221_error_code_t` return values instead of
Python objects and exceptions.

| Python (EasyMCP2221 v1.8.4)                      | libeasymcp2221 v2 C API                                                                                                                                                      | Notes                                                                                                                                       |
| ------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| `Device(...)` (open, reuse, `scan_serial`)       | `mcp2221_open_scan(vid, pid, devnum, usbserial, usb_read_timeout_ms, retries, debug, trace, scan_serial)` / `mcp2221_open_simple(vid, pid, devnum, usbserial, i2c_speed_hz)` | Opens or reuses an MCP2221 handle; optional flash-serial scanning is supported.                                                             |
| `Device.close()`                                 | `mcp2221_close(device)`                                                                                                                                                      | Releases the MCP2221 handle and restores an internally detached kernel driver when applicable.                                              |
| `Device.send_cmd(buf)`                           | `mcp2221_send_cmd(device, buf, len, response)`                                                                                                                               | Sends a raw command using fixed-size USB reports internally.                                                                                |
| `Device._i2c_status()`                           | `mcp2221_i2c_status(device, status)`                                                                                                                                         | Returns a snapshot of the MCP2221 I2C engine, including the `confused` and `initialized` compatibility heuristics.                          |
| `Device._i2c_release()`                          | `mcp2221_i2c_release(device)`                                                                                                                                                | Cancels or releases a stuck I2C transaction.                                                                                                |
| `Device.I2C_speed(speed)`                        | `mcp2221_i2c_set_speed(device, i2c_speed_hz)`                                                                                                                                | Uses Python-compatible ties-to-even rounding when calculating the clock divider.                                                            |
| `Device.I2C_write(addr, data, kind, timeout_ms)` | `mcp2221_i2c_write_ex(device, addr, data, len, kind, i2c_timeout_ms)`                                                                                                        | Performs an I2C write with an explicit transfer timeout.                                                                                    |
| `Device.I2C_read(addr, size, kind, timeout_ms)`  | `mcp2221_i2c_read_ex(device, addr, data, len, kind, i2c_timeout_ms)`                                                                                                         | Returns `MCP2221_ERR_I2C_SHORT_READ` if the device reports completion before all requested bytes are received.                              |
| `Device.GPIO_write(gp0..gp3)`                    | `mcp2221_gpio_write(device, ...)`                                                                                                                                            | `MCP2221_GPIO_KEEP` preserves an output value. Use `MCP2221_CONFIG_KEEP` for SRAM configuration fields.                                     |
| `Device.GPIO_read()`                             | `mcp2221_gpio_read(device, ...)` / `mcp2221_gpio_read_mask(device, ...)`                                                                                                     | The mask variant represents non-GPIO pins separately, corresponding to Python `None`.                                                       |
| `Device.GPIO_poll()`                             | `mcp2221_gpio_poll_events(device, ...)` / `mcp2221_gpio_poll(device, ...)`                                                                                                   | The event variant returns rise/fall records. The simple variant returns an error code and reports per-pin changes through its output array. |
| `Device.set_pin_function(gp0..gp3, out0..out3)`  | `mcp2221_pin_set_functions(device, ...)` / `mcp2221_pin_set_function(device, ...)`                                                                                           | The batch form mirrors the Python operation; an output value is valid only for a GPIO output.                                               |
| `Device.SRAM_config(...)`                        | `mcp2221_sram_config(device, ...)`                                                                                                                                           | Applies SRAM settings while preserving GPIO output bits through the internal GPIO cache.                                                    |
| `Device.ADC_config(ref, vdd)`                    | `mcp2221_analog_set_vdd(device, volts)` / `mcp2221_adc_config(device, ref_str)`                                                                                              | Configure VDD separately when the ADC uses VDD as its reference.                                                                            |
| `Device.ADC_read(norm, volts)`                   | `mcp2221_adc_read_raw(device, out)` / `mcp2221_adc_read_normalized(device, out)` / `mcp2221_adc_read_volts(device, out)`                                                     | Provides raw, normalized or voltage-based readings.                                                                                         |
| `Device.DAC_config(ref, out, vdd)`               | `mcp2221_analog_set_vdd(device, volts)` / `mcp2221_dac_config_out(device, ref_str, out_code)` / `mcp2221_dac_config(device, ref_str)`                                        | Configure VDD separately when the DAC uses VDD as its reference.                                                                            |
| `Device.DAC_write(out, norm, volts)`             | `mcp2221_dac_write_raw(device, code)` / `mcp2221_dac_write_normalized(device, value)` / `mcp2221_dac_write_volts(device, volts)`                                             | Supports raw, normalized and voltage-based output.                                                                                          |
| `Device.clock_config(duty, freq)`                | `mcp2221_clock_config(device, duty_percent, freq_str)`                                                                                                                       | Accepts the supported duty-cycle percentages and frequency strings.                                                                         |
| `Device.IOC_read()`                              | `mcp2221_ioc_read(device, flag)`                                                                                                                                             | Reads the interrupt-on-change flag.                                                                                                         |
| `Device.IOC_clear()`                             | `mcp2221_ioc_clear(device)`                                                                                                                                                  | Clears the interrupt-on-change flag.                                                                                                        |
| `Device.IOC_config(edge)`                        | `mcp2221_ioc_config(device, edge_str)`                                                                                                                                       | Accepts `none`, `rising`, `falling` or `both`.                                                                                              |
| `Device.read_flash_info()` and parsing           | `mcp2221_flash_read_info(device, info)`                                                                                                                                      | Reads the flash sections and performs best-effort conversion of USB UTF-16LE strings to null-terminated UTF-8 strings.                      |
| `Device.save_config()`                           | `mcp2221_flash_save_config(device)`                                                                                                                                          | Saves the current SRAM chip and GPIO configuration to flash.                                                                                |
| `Device.enable_power_management(enable)`         | `mcp2221_usb_set_remote_wakeup(device, enable)`                                                                                                                              | Stages the USB Remote Wake-up capability; save the configuration and re-enumerate the device for it to take effect.                         |
| `I2C_Slave.I2C_Slave`                            | `mcp2221_i2c_slave_init(slave, device, ...)` and `mcp2221_i2c_slave_*()`                                                                                                     | Initializes a caller-owned context; no allocation is performed.                                                                             |
| `smbus.SMBus` (subset)                           | `mcp2221_smbus_init(bus, device, ...)`, `mcp2221_smbus_close(bus)` and `mcp2221_smbus_*()`                                                                                   | Supports a subset of the Python SMBus interface and distinguishes borrowed from internally opened device handles.                           |

## USB power attributes and Remote Wake-up

`mcp2221_usb_set_remote_wakeup()` configures whether the MCP2221 advertises
USB Remote Wake-up capability. A nonzero `enable` value enables the feature;
zero disables it. EasyMCP2221 exposes the same MCP2221 capability through
`Device.enable_power_management()`. Actual wake-up additionally depends on GP1
interrupt-on-change configuration and the host operating system allowing the
device to wake the system.

`mcp2221_usb_set_self_powered()` configures whether the device advertises
itself as self-powered. This setting does not change the physical power source;
it must only be enabled for hardware that is actually self-powered. This is a
libeasymcp2221 extension beyond EasyMCP2221's high-level power-management API.

`mcp2221_usb_set_requested_current()` configures the USB bus current advertised
by the device. The MCP2221 stores this field in 2 mA units, so the public API
accepts even values from 0 through 500 mA. This setting describes the device to
the USB host; it does not electrically limit, regulate or switch current.
`mcp2221_usb_set_requested_current(device, 100)` therefore means 100 mA; the
library converts that to the MCP2221 register value 50 internally. This setter
is also a libeasymcp2221 extension beyond the original high-level Python API.

These functions configure USB enumeration attributes stored by the MCP2221.
They do **not**:

- control host-side USB autosuspend or selective suspend;
- enable or disable power to a USB port;
- change the physical power source of the MCP2221 hardware;
- guarantee that the host operating system will permit Remote Wake-up.

All three setters stage enumeration-time settings. Call
`mcp2221_flash_save_config()` to persist them, then reset or reconnect the
MCP2221 so the USB host enumerates the device again.

There are therefore three distinct configuration states:

```text
mcp2221_usb_set_*()
        |
        v
staged / pending configuration
        |
        | mcp2221_flash_save_config()
        v
persistent MCP2221 flash configuration
        |
        | reset, reconnect or other USB re-enumeration
        v
active USB enumeration configuration seen by the host
```

### Effective getter semantics

The `mcp2221_usb_get_*()` functions return the **effective library
configuration**:

1. if a setter has staged a value, the staged value is returned;
2. otherwise the current value is read from MCP2221 flash.

This means a setter followed immediately by its getter returns the newly staged
value even before `mcp2221_flash_save_config()`:

```c
mcp2221_usb_set_remote_wakeup(device, 1);
mcp2221_usb_get_remote_wakeup(device, &enabled);
/* enabled == 1, even before flash_save_config() */
```

After a successful `mcp2221_flash_save_config()`, the pending state is cleared
and subsequent getters resolve the value from flash. If saving fails, the
pending state remains staged so the save can be retried; the getters therefore
continue to report the staged value.

The getters do not report the currently active host-side USB state. In
particular, `mcp2221_usb_get_remote_wakeup()` does not indicate whether the
operating system currently permits the MCP2221 to wake the computer.

### Remote Wake-up versus wake-up source

`mcp2221_usb_set_remote_wakeup()` only makes the MCP2221 advertise Remote
Wake-up capability. For an actual wake event, configure a suitable source such
as GP1 interrupt-on-change and ensure that the host operating system permits
the device to wake the system. Those mechanisms are separate from the USB
enumeration attribute itself.

## C API naming scheme

The public v2 API follows one naming scheme:

- functions: `mcp2221_<domain>_<verb>[_object]()`
- types: `mcp2221_<domain>_<name>_t`
- enum constants and public macros: `MCP2221_<DOMAIN>_<NAME>`

The compatibility aliases and unprefixed public headers provided by the 1.x
series were removed in version 2. Applications upgrading from 1.x should
follow [`MIGRATION.md`](MIGRATION.md).

## I2C slave context storage

`mcp2221_i2c_slave_t` is a caller-owned public value type, not an opaque,
library-allocated handle. Applications may allocate it on the stack, statically
or as part of another structure. Initialize the context with
`mcp2221_i2c_slave_init()` and keep the referenced `mcp2221_t` device open for
as long as the slave context is used.

## I2C status fields

`mcp2221_i2c_status()` fills `mcp2221_i2c_status_t` with a snapshot of the
MCP2221 I2C engine:

| Field         | Meaning                                                                                   |
| ------------- | ----------------------------------------------------------------------------------------- |
| `rlen`        | Requested transfer length reported by the device.                                         |
| `txlen`       | Number of bytes transmitted by the I2C engine.                                            |
| `div`         | Raw MCP2221 I2C clock-divider register value.                                             |
| `ack`         | Raw ACK-status bit mask from bit 6; the value is `0` or `0x40`, not a normalized boolean. |
| `st`          | Raw MCP2221 internal I2C state code.                                                      |
| `scl` / `sda` | Sampled bus-line levels, each `0` or `1`.                                                 |
| `confused`    | EasyMCP2221-compatible heuristic indicating an inconsistent I2C engine state.             |
| `initialized` | EasyMCP2221-compatible heuristic indicating that the I2C engine is initialized.           |

Applications should not interpret `ack`, `st` or `div` as normalized values.
These fields expose the corresponding raw MCP2221 status values.

## I2C timeout variants

The `_ex` and `_simple` suffixes describe timeout handling:

| Function                                                   | Timeout behavior                                                                                             |
| ---------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| `mcp2221_i2c_write_ex()` / `mcp2221_i2c_read_ex()`         | The caller supplies `i2c_timeout_ms` explicitly.                                                             |
| `mcp2221_i2c_write_simple()` / `mcp2221_i2c_read_simple()` | The library uses the device's configured `usb_read_timeout_ms` when it is positive; otherwise it uses 20 ms. |

The suffixes do not change the I2C transfer kind or payload semantics. New code
that needs deterministic per-operation timeout behavior should use the `_ex`
variants.

## I2C transfer kinds

Use `mcp2221_i2c_kind_t`:

| Kind                              | Meaning                                              |
| --------------------------------- | ---------------------------------------------------- |
| `MCP2221_I2C_KIND_NORMAL`         | Normal transfer with stop condition.                 |
| `MCP2221_I2C_KIND_REPEATED_START` | Repeated-start transfer. Valid for reads and writes. |
| `MCP2221_I2C_KIND_NO_STOP`        | Write without stop condition. Valid for writes.      |

## Keep sentinels

`MCP2221_GPIO_KEEP` is the domain-specific preserve sentinel for `mcp2221_gpio_write_t` fields. `MCP2221_CONFIG_KEEP` is the domain-specific preserve sentinel for SRAM configuration structures. They currently share the numeric value `-1`, but applications should use the
matching domain constant instead of hard-coding `-1` or reusing another
domain's sentinel.

## Error handling

Most public functions return `MCP2221_ERR_OK` on success or another
`mcp2221_error_code_t` value on failure. Functions returning payload sizes
generally use output parameters for lengths. GPIO read helpers report pin
states through caller-provided arrays while returning an error code.

Some polling functions use `int` because their success values are not always
represented solely by `mcp2221_error_code_t`. `mcp2221_gpio_poll_events()`
returns a non-negative event count on success and a negative error code on
failure. `mcp2221_gpio_poll()` returns zero on success, reports changes through
its output array and returns a negative error code on failure.

Important error codes include:

| Error code                   | Typical meaning                                                 |
| ---------------------------- | --------------------------------------------------------------- |
| `MCP2221_ERR_USB`            | A USB or libusb operation failed.                               |
| `MCP2221_ERR_TIMEOUT`        | A USB or I2C operation timed out.                               |
| `MCP2221_ERR_NOT_ACK`        | An I2C address or data byte was not acknowledged.               |
| `MCP2221_ERR_LOW_SCL`        | The I2C SCL line is held low.                                   |
| `MCP2221_ERR_LOW_SDA`        | The I2C SDA line is held low.                                   |
| `MCP2221_ERR_INVALID`        | An argument or requested value is invalid.                      |
| `MCP2221_ERR_I2C`            | A generic I2C state-machine failure occurred.                   |
| `MCP2221_ERR_I2C_SHORT_READ` | An I2C read completed before all requested bytes were received. |
| `MCP2221_ERR_FLASH_READ`     | A flash read operation failed.                                  |
| `MCP2221_ERR_FLASH_WRITE`    | A flash write operation failed.                                 |
| `MCP2221_ERR_FLASH_PASSWD`   | The flash access password was rejected.                         |
| `MCP2221_ERR_GPIO_MODE`      | A pin is not configured for the requested GPIO operation.       |

Use `mcp2221_error_code_to_string()` to convert an error code to its stable
symbolic name.

## Resource ownership

- `mcp2221_open*()` returns an owned `mcp2221_t *`; call `mcp2221_close()` when done.
- `mcp2221_smbus_init(bus, existing_mcp, ...)` borrows `existing_mcp` when it is non-NULL. `mcp2221_smbus_close()` does not close borrowed MCP2221 handles.
- If `mcp2221_smbus_init()` opens its own MCP2221 handle, `mcp2221_smbus_close()` releases it.

## SMBus block size

`MCP2221_I2C_SMBUS_BLOCK_MAX` is 255 for EasyMCP2221 compatibility. EasyMCP2221's SMBus layer treats the block length as a one-byte value, so the compatibility limit is 255 payload bytes rather than the classic SMBus 32-byte payload limit.

The block-read helpers internally receive one extra byte for the length field and return only payload bytes through the caller-provided output buffer:

- `mcp2221_smbus_read_block_data()` writes up to `MCP2221_I2C_SMBUS_BLOCK_MAX` payload bytes to `buffer` and stores the actual payload length in `*length`.
- `mcp2221_smbus_block_process_call()` writes up to `MCP2221_I2C_SMBUS_BLOCK_MAX` response payload bytes to `response` and stores the actual payload length in `*resp_len`.

For devices or applications that require strict SMBus block semantics, cap payload lengths at 32 bytes at the application level.

## Thread safety

`mcp2221_open*()` and `mcp2221_close()` are internally serialized. This protects the shared libusb context, the reference counter and the device catalog used to reuse handles for the same physical device.

I2C, GPIO, SRAM, flash and SMBus operations on an already opened `mcp2221_t *` are not serialized by libeasymcp2221. Use one handle from one thread at a time, or protect shared handles with an application-level mutex.

## Macro naming

Public constants and macros use the `MCP2221_*` prefix.

Examples include:

| Purpose                                | Public name                        |
| -------------------------------------- | ---------------------------------- |
| Default USB vendor ID                  | `MCP2221_DEV_DEFAULT_VID`          |
| Default USB product ID                 | `MCP2221_DEV_DEFAULT_PID`          |
| USB report size                        | `MCP2221_PACKET_SIZE`              |
| Maximum 7-bit I2C address              | `MCP2221_I2C_ADDR_7BIT_MAX`        |
| Maximum SMBus-compatible block payload | `MCP2221_I2C_SMBUS_BLOCK_MAX`      |
| Preserve an SRAM configuration value   | `MCP2221_CONFIG_KEEP`              |
| GPIO polling rise-event mask           | `MCP2221_GPIO_POLL_MASK_RISE(pin)` |
| GPIO polling fall-event mask           | `MCP2221_GPIO_POLL_MASK_FALL(pin)` |

## Differences and unsupported features

- Python exceptions are represented by explicit `mcp2221_error_code_t` return values.
- EasyMCP2221 accepts `vdd` as an optional argument of ADC and DAC methods.
  The C API stores it separately with `mcp2221_analog_set_vdd()`.