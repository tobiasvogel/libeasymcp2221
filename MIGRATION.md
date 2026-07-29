# libeasymcp2221 Migration Guide

This guide summarizes the API cleanup introduced in the 1.1 series. Existing source code should keep compiling through compatibility aliases, but new code should use the preferred names.

## API naming cleanup

Preferred public names use one consistent scheme:

- functions: `mcp2221_<domain>_<verb>[_object]()`
- types: `mcp2221_<domain>_<name>_t`
- public constants/macros: `MCP2221_<DOMAIN>_<NAME>`

The older names remain available as compatibility aliases for the 1.x series. They are marked with `MCP2221_DEPRECATED("use ...")` where compiler-portable deprecation is possible. Compatibility aliases may be removed in the next major version, currently planned as 2.0.

To suppress warnings temporarily while migrating existing code, define this before including libeasymcp2221 headers:

```c
#define MCP2221_NO_DEPRECATED_WARNINGS
```

New code, examples and documentation should use the preferred names.

## Common type and function replacements

| Legacy name | Preferred name |
|---|---|
| `MCP2221` | `mcp2221_t` |
| `I2C_Slave` | `mcp2221_i2c_slave_t` |
| `SMBus` | `mcp2221_smbus_t` |
| `mcp2221_i2c_speed()` | `mcp2221_i2c_set_speed()` |
| `mcp2221_i2c_write()` | `mcp2221_i2c_write_ex()` |
| `mcp2221_i2c_read()` | `mcp2221_i2c_read_ex()` |
| `mcp2221_create_i2c_slave()` | `mcp2221_i2c_slave_create()` |
| `i2c_slave_init()` | `mcp2221_i2c_slave_init()` |
| `i2c_slave_read_register()` | `mcp2221_i2c_slave_read_register()` |
| `i2c_slave_read()` | `mcp2221_i2c_slave_read()` |
| `i2c_slave_write_register()` | `mcp2221_i2c_slave_write_register()` |
| `i2c_slave_write()` | `mcp2221_i2c_slave_write()` |
| `smbus_init()` | `mcp2221_smbus_init()` |
| `smbus_close()` | `mcp2221_smbus_close()` |
| `smbus_*()` | `mcp2221_smbus_*()` |
| `mcp2221_set_pin_function()` | `mcp2221_pin_set_function()` |
| `mcp2221_set_pin_functions()` | `mcp2221_pin_set_functions()` |

## Parameter name clarifications

Some parameter names were clarified without changing their meaning:

| Old wording | Preferred wording | Meaning |
|---|---|---|
| `clock_hz` / `speed_hz` | `i2c_speed_hz` | I2C bus speed in Hz. |
| `read_timeout_ms` | `usb_read_timeout_ms` | USB report read timeout in milliseconds. |
| `timeout_ms` | `i2c_timeout_ms` | I2C transfer timeout in milliseconds. |

## Macro and header-guard names

Public macros now use the `MCP2221_*` prefix. Legacy macro names remain available as aliases for the 1.x series, but new code should use the prefixed names. Include guards in public headers also use the `MCP2221_*_H` form.

| Legacy macro | Preferred macro |
|---|---|
| `DEV_DEFAULT_VID` | `MCP2221_DEV_DEFAULT_VID` |
| `DEV_DEFAULT_PID` | `MCP2221_DEV_DEFAULT_PID` |
| `PACKET_SIZE` | `MCP2221_PACKET_SIZE` |
| `I2C_ADDR_7BIT_MAX` | `MCP2221_I2C_ADDR_7BIT_MAX` |
| `I2C_SMBUS_BLOCK_MAX` | `MCP2221_I2C_SMBUS_BLOCK_MAX` |
| `MCP_CONFIG_KEEP` | `MCP2221_CONFIG_KEEP` |
| `MCP_GPIO_POLL_MASK_RISE(pin)` | `MCP2221_GPIO_POLL_MASK_RISE(pin)` |
| `MCP_GPIO_POLL_MASK_FALL(pin)` | `MCP2221_GPIO_POLL_MASK_FALL(pin)` |

## SMBus block size

`MCP2221_I2C_SMBUS_BLOCK_MAX` remains 255 for compatibility with EasyMCP2221, whose SMBus helper uses a one-byte length field. The constant describes the maximum payload length accepted by the compatibility helpers.

This differs from the classic SMBus block size of 32 payload bytes. Code that intentionally targets strict SMBus devices should enforce a 32-byte application-level limit, while code that follows EasyMCP2221/MCP2221 behavior can use the 255-byte compatibility limit.

## I2C read error behavior

`mcp2221_i2c_read_ex()` now treats a completed read with fewer bytes than requested as `MCP_ERR_I2C_SHORT_READ`. Code that previously treated a successful but short transfer as valid should either request the exact expected size or handle `MCP_ERR_I2C_SHORT_READ` explicitly.

## Flash read/write error behavior

Flash read failures are reported as `MCP_ERR_FLASH_READ`. Flash write failures remain `MCP_ERR_FLASH_WRITE`. This makes diagnostics more precise for helpers such as `mcp2221_flash_read_info()` and `mcp2221_flash_save_config()`.

## SMBus ownership

`mcp2221_smbus_init()` has explicit ownership semantics:

- If `existing_mcp` is non-NULL, the SMBus wrapper borrows that handle. `mcp2221_smbus_close()` does not close it.
- If `existing_mcp` is NULL, the SMBus wrapper opens its own MCP2221 handle. `mcp2221_smbus_close()` closes it.

Always call `mcp2221_smbus_close()` for initialized SMBus wrappers so that internally-owned handles are released.

## Removal policy

Compatibility aliases are guaranteed to remain available for the 1.x series. They should not be removed in a patch or minor release. Removal is reserved for a major release and should be called out in the release notes.
