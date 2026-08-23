# Migrating from libeasymcp2221 1.x to 2.x

libeasymcp2221 2.x removes the compatibility aliases and unprefixed public
headers that were retained during the 1.x series.

Applications using the canonical `mcp2221_*` function names,
`mcp2221_*_t` type names, `MCP2221_*` constants and namespaced include paths
will usually require only minor changes.

This guide summarizes the source-level changes required when upgrading an
existing 1.x application.

## Overview

The main changes in version 2 are:

* legacy type aliases were removed;
* deprecated function wrappers were removed;
* unprefixed constant and macro aliases were removed;
* unprefixed compatibility headers were removed;
* the old error-wrapper API was removed;
* public declarations consistently use `mcp2221_error_code_t`;
* installed headers are included through the `libeasymcp2221/` directory.

The public v2 naming scheme is:

* functions: `mcp2221_<domain>_<verb>[_object]()`
* types: `mcp2221_<domain>_<name>_t`
* constants and macros: `MCP2221_<DOMAIN>_<NAME>`

## Installed header paths

Version 2 installs public headers under:

```text
<prefix>/include/libeasymcp2221/
```

Applications should therefore use namespaced include paths.

Before:

```c
#include <mcp2221.h>
#include <constants.h>
#include <i2c_slave.h>
#include <smbus.h>
```

After:

```c
#include <libeasymcp2221/mcp2221.h>
#include <libeasymcp2221/mcp2221_constants.h>
#include <libeasymcp2221/mcp2221_i2c_slave.h>
#include <libeasymcp2221/mcp2221_smbus.h>
```

The public API is available through the namespaced `mcp2221_*.h` headers.
Version 2 also provides a convenience umbrella header:

```c
#include <libeasymcp2221/libeasymcp2221.h>
```

The old unprefixed compatibility headers are no longer installed.

## Removed compatibility headers

Replace the removed 1.x compatibility headers as follows:

| Removed 1.x header | v2 header                                           |
| ------------------ | --------------------------------------------------- |
| `constants.h`      | `mcp2221_constants.h`                               |
| `error_codes.h`    | `mcp2221_error_codes.h`                             |
| `exceptions.h`     | `mcp2221_errors.h`                                  |
| `i2c_slave.h`      | `mcp2221_i2c_slave.h`                               |
| `smbus.h`          | `mcp2221_smbus.h`                                   |

When using an installed library, prefix these paths with
`libeasymcp2221/`.

## Device opening

Version 2 changes the `mcp2221_open*()` family from pointer-returning helpers
to the library-wide error-code convention.

Before:

```c
mcp2221_t *dev =
    mcp2221_open_simple(vid, pid, devnum, usbserial, i2c_speed_hz);
```

After:

```c
mcp2221_t *dev = NULL;
mcp2221_error_code_t err =
    mcp2221_open_simple(
        vid, pid, devnum, usbserial, i2c_speed_hz, &dev);
```

On failure, the function returns the detailed `mcp2221_error_code_t` and
leaves `dev == NULL`. The same output-parameter contract applies to
`mcp2221_open()`, `mcp2221_open_scan()` and
`mcp2221_open_simple_scan()`.

## Removed type aliases

Version 2 uses the canonical names directly.

| Removed 1.x name      | v2 name                     |
| --------------------- | --------------------------- |
| `MCP2221`             | `mcp2221_t`                 |
| `I2C_Slave`           | `mcp2221_i2c_slave_t`       |
| `SMBus`               | `mcp2221_smbus_t`           |
| `mcp_err_t`           | `mcp2221_error_code_t`      |
| `MCP2221_FlashInfo`   | `mcp2221_flash_info_t`      |
| `MCP2221_SRAM_Config` | `mcp2221_sram_config_t`     |
| `MCP_GPIO_PollState`  | `mcp2221_gpio_poll_state_t` |
| `MCP_GPIO_Change`     | `mcp2221_gpio_change_t`     |

Other `MCP_*` and mixed-case structure aliases were removed in the same way.
Use the corresponding `mcp2221_*_t` type from the public v2 headers.

Before:

```c
MCP2221 *dev;
mcp_err_t err;
MCP2221_FlashInfo info;
```

After:

```c
mcp2221_t *dev;
mcp2221_error_code_t err;
mcp2221_flash_info_t info;
```

## Removed function aliases

Version 2 removes the deprecated wrappers retained by the 1.x series.

| Removed 1.x function          | v2 function                                              |
| ----------------------------- | -------------------------------------------------------- |
| `mcp2221_i2c_speed()`         | `mcp2221_i2c_set_speed()`                                |
| `mcp2221_i2c_write()`         | `mcp2221_i2c_write_ex()` or `mcp2221_i2c_write_simple()` |
| `mcp2221_i2c_read()`          | `mcp2221_i2c_read_ex()` or `mcp2221_i2c_read_simple()`   |
| `mcp2221_i2c_slave_create()`  | `mcp2221_i2c_slave_init()`                               |
| `mcp2221_create_i2c_slave()`  | `mcp2221_i2c_slave_init()`                               |
| `i2c_slave_init()`            | `mcp2221_i2c_slave_init()`                               |
| `i2c_slave_read_register()`   | `mcp2221_i2c_slave_read_register()`                      |
| `i2c_slave_read()`            | `mcp2221_i2c_slave_read()`                               |
| `i2c_slave_write_register()`  | `mcp2221_i2c_slave_write_register()`                     |
| `i2c_slave_write()`           | `mcp2221_i2c_slave_write()`                              |
| `smbus_init()`                | `mcp2221_smbus_init()`                                   |
| `smbus_close()`               | `mcp2221_smbus_close()`                                  |
| `smbus_*()`                   | `mcp2221_smbus_*()`                                      |
| `mcp2221_set_pin_function()`  | `mcp2221_pin_set_function()`                             |
| `mcp2221_set_pin_functions()` | `mcp2221_pin_set_functions()`                            |
| `mcp_error_code_to_string()`  | `mcp2221_error_code_to_string()`                         |

## I2C read and write variants

The old generic I2C read and write wrappers were replaced by explicit
variants.

Use:

```c
mcp2221_i2c_write_ex(
    dev,
    address,
    data,
    length,
    kind,
    i2c_timeout_ms
);
```

and:

```c
mcp2221_i2c_read_ex(
    dev,
    address,
    data,
    length,
    kind,
    i2c_timeout_ms
);
```

when the application needs an explicit I2C stall/progress watchdog value.
Values less than or equal to zero select the 20 ms fallback.

The watchdog is not a hard wall-clock limit for a complete multi-chunk
transaction. Writes apply it to each chunk/progress phase. Reads restart it
only when actual read data is received.

The convenience variants:

```c
mcp2221_i2c_write_simple(...)
mcp2221_i2c_read_simple(...)
```

derive the I2C watchdog from the device's configured USB read timeout when it
is positive. Otherwise they use a 20 ms fallback.

The transfer kind is represented by `mcp2221_i2c_kind_t`:

```c
MCP2221_I2C_KIND_NORMAL
MCP2221_I2C_KIND_REPEATED_START
MCP2221_I2C_KIND_NO_STOP
```

## I2C slave initialization

`mcp2221_i2c_slave_t` is caller-owned storage. The library does not allocate
or free the context.

The parameter order changed between the older 1.x convenience wrapper and the
v2 initializer. In particular, the device handle and slave context exchanged
positions.

Version 1.x:

```c
mcp_err_t mcp2221_create_i2c_slave(
    MCP2221 *dev,
    I2C_Slave *slave,
    uint8_t addr,
    int force,
    uint32_t speed_hz,
    int reg_bytes,
    const char *reg_byteorder
);
```

Version 2:

```c
mcp2221_error_code_t mcp2221_i2c_slave_init(
    mcp2221_i2c_slave_t *slave,
    mcp2221_t *dev,
    uint8_t addr,
    int force,
    uint32_t i2c_speed_hz,
    int reg_bytes,
    mcp2221_i2c_byte_order_t reg_byteorder
);
```

Register byte order is no longer passed as a string. Replace the old values as
follows:

| Previous argument | v2 argument                         |
| ----------------- | ----------------------------------- |
| `"big"`           | `MCP2221_I2C_BYTE_ORDER_BIG`        |
| `"little"`        | `MCP2221_I2C_BYTE_ORDER_LITTLE`     |
| `NULL`            | `MCP2221_I2C_BYTE_ORDER_DEFAULT`    |

For `mcp2221_i2c_slave_init()`, `DEFAULT` selects big endian. For
`mcp2221_i2c_slave_read_register()` and
`mcp2221_i2c_slave_write_register()`, `DEFAULT` inherits the byte order stored
in the slave context.

The v2 call therefore begins with the caller-owned slave context, followed by
the borrowed MCP2221 device handle:

```c
mcp2221_i2c_slave_t slave;

mcp2221_error_code_t err =
    mcp2221_i2c_slave_init(
        &slave,
        dev,
        address,
        force,
        i2c_speed_hz,
        reg_bytes,
        MCP2221_I2C_BYTE_ORDER_BIG
    );
```

Do not mechanically rename `mcp2221_create_i2c_slave()` to
`mcp2221_i2c_slave_init()` without also reordering the first two arguments.

The device handle must remain open for as long as the slave context is used.

### I2C slave presence checks

For new v2 code, prefer the error-returning presence helper when the
application needs to distinguish an address NACK from transport or protocol
failures:

```c
int present;
mcp2221_error_code_t err =
    mcp2221_i2c_slave_check_present(&slave, &present);

if (err != MCP2221_ERR_OK) {
    /* transport, timeout or other I2C error */
} else if (!present) {
    /* slave did not acknowledge its address */
}
```

An address NACK is reported as `MCP2221_ERR_OK` with `present == 0`.
Transport, timeout and other I2C errors are returned to the caller.

`mcp2221_i2c_slave_is_present()` remains available as a Boolean-style
compatibility helper. It returns `1` only when the slave ACKs and cannot
distinguish an address NACK from another error.

## SMBus ownership

`mcp2221_smbus_t` is also initialized in caller-provided storage.

When an existing `mcp2221_t *` is passed to `mcp2221_smbus_init()`, the SMBus
context borrows that handle. `mcp2221_smbus_close()` does not close borrowed
device handles.

When `mcp2221_smbus_init()` opens the device itself,
`mcp2221_smbus_close()` releases the internally owned handle.

Always call `mcp2221_smbus_close()` for an initialized SMBus context.

## Removed constant and macro aliases

Version 2 exposes only the `MCP2221_*` names.

| Removed 1.x name               | v2 name                            |
| ------------------------------ | ---------------------------------- |
| `DEV_DEFAULT_VID`              | `MCP2221_DEV_DEFAULT_VID`          |
| `DEV_DEFAULT_PID`              | `MCP2221_DEV_DEFAULT_PID`          |
| `PACKET_SIZE`                  | `MCP2221_PACKET_SIZE`              |
| `I2C_ADDR_7BIT_MAX`            | `MCP2221_I2C_ADDR_7BIT_MAX`        |
| `I2C_SMBUS_BLOCK_MAX`          | `MCP2221_I2C_SMBUS_BLOCK_MAX`      |
| `MCP_CONFIG_KEEP`              | `MCP2221_CONFIG_KEEP`              |
| `MCP_GPIO_POLL_MASK_RISE(pin)` | `MCP2221_GPIO_POLL_MASK_RISE(pin)` |
| `MCP_GPIO_POLL_MASK_FALL(pin)` | `MCP2221_GPIO_POLL_MASK_FALL(pin)` |

Use `MCP2221_GPIO_KEEP` when preserving a GPIO output value and
`MCP2221_CONFIG_KEEP` when preserving a field in an SRAM configuration
structure.

Both constants currently use the numeric value `-1`, but they belong to
different API domains and should not be interchanged.

## Stricter SRAM configuration validation

Version 2 validates `mcp2221_sram_config_t` fields against their documented
values before accessing the device. Out-of-range values that older code may
have relied on being truncated, masked or treated as true are now rejected with
`MCP2221_ERR_INVALID`.

Applications should use the public `MCP2221_*` constants and documented ranges
directly. In particular:

- Boolean-like SRAM fields accept only `0`, `1` or
  `MCP2221_CONFIG_KEEP`;
- DAC values must be in the range `0..31` or use
  `MCP2221_CONFIG_KEEP`;
- clock dividers must use `MCP2221_CLK_DIV_1` through
  `MCP2221_CLK_DIV_7` or `MCP2221_CONFIG_KEEP`;
- ADC/DAC VRM and clock-duty fields must use their corresponding public
  constants;
- GPIO alternate-function selections must be valid for the selected GP pin.

Code that previously passed arbitrary nonzero values, out-of-range DAC values
or raw bit patterns should be updated to use the documented v2 constants.

## Error handling

Version 2 uses `mcp2221_error_code_t` for functions whose return value
represents an error code.

Before:

```c
mcp_err_t err = mcp2221_i2c_set_speed(dev, 100000);

if (err != MCP_ERR_OK) {
    fprintf(stderr, "%s\n", mcp_error_code_to_string(err));
}
```

After:

```c
mcp2221_error_code_t err =
    mcp2221_i2c_set_speed(dev, 100000);

if (err != MCP2221_ERR_OK) {
    fprintf(stderr, "%s\n",
            mcp2221_error_code_to_string(err));
}
```

Common replacements include:

| Removed 1.x constant     | v2 constant                  |
| ------------------------ | ---------------------------- |
| `MCP_ERR_OK`             | `MCP2221_ERR_OK`             |
| `MCP_ERR_USB`            | `MCP2221_ERR_USB`            |
| `MCP_ERR_TIMEOUT`        | `MCP2221_ERR_TIMEOUT`        |
| `MCP_ERR_NOT_ACK`        | `MCP2221_ERR_NOT_ACK`        |
| `MCP_ERR_LOW_SCL`        | `MCP2221_ERR_LOW_SCL`        |
| `MCP_ERR_LOW_SDA`        | `MCP2221_ERR_LOW_SDA`        |
| `MCP_ERR_INVALID`        | `MCP2221_ERR_INVALID`        |
| `MCP_ERR_I2C`            | `MCP2221_ERR_I2C`            |
| `MCP_ERR_I2C_SHORT_READ` | `MCP2221_ERR_I2C_SHORT_READ` |
| `MCP_ERR_FLASH_READ`     | `MCP2221_ERR_FLASH_READ`     |
| `MCP_ERR_FLASH_WRITE`    | `MCP2221_ERR_FLASH_WRITE`    |
| `MCP_ERR_FLASH_PASSWD`   | `MCP2221_ERR_FLASH_PASSWD`   |
| `MCP_ERR_GPIO_MODE`      | `MCP2221_ERR_GPIO_MODE`      |

Version 2 also adds more specific device-open and command/protocol errors:

```c
MCP2221_ERR_NOT_FOUND
MCP2221_ERR_NO_MEMORY
MCP2221_ERR_ACCESS
MCP2221_ERR_BUSY
MCP2221_ERR_USB_INIT
MCP2221_ERR_USB_ENUM
MCP2221_ERR_USB_OPEN
MCP2221_ERR_USB_CLAIM
MCP2221_ERR_COMMAND_FAILED
MCP2221_ERR_PROTOCOL
```

`MCP2221_ERR_COMMAND_FAILED` means that a syntactically valid MCP2221 command
was rejected by the device or reported as failed.

`MCP2221_ERR_PROTOCOL` means that a response violated the expected MCP2221
protocol contract, for example because the command echo did not match the
request.

Use `mcp2221_error_code_to_string()` to obtain the symbolic name of an error
code.

## Removed error-wrapper API

The old `mcp_error_t` message-wrapper API was removed in version 2.

Removed functions include:

```c
mcp_error_init()
mcp_error_set_message()
mcp_error_clear()
mcp_error_to_string_dup()
```

libeasymcp2221 uses explicit error codes rather than dynamically managed error
objects. Applications should store a `mcp2221_error_code_t` and convert it
with `mcp2221_error_code_to_string()` when a textual representation is needed.

## Return-value conventions

Most public functions return `MCP2221_ERR_OK` on success or another
`mcp2221_error_code_t` value on failure.

Payload sizes are generally returned through output parameters.

`mcp2221_gpio_poll()` follows the normal error-code convention and reports
changes through an output array.

`mcp2221_gpio_poll_events()` is an exception because its non-negative return
value is the number of events written. Negative values are error codes.

Do not assume that every public `int` return value is a Boolean result.

## GPIO polling types

Replace the old GPIO polling aliases with the v2 names.

Before:

```c
MCP_GPIO_PollState state;
MCP_GPIO_Change changes[4];
```

After:

```c
mcp2221_gpio_poll_state_t state;
mcp2221_gpio_change_t changes[4];
```

Initialize polling state before the first call:

```c
mcp2221_gpio_poll_init(&state);
```

The first polling call initializes the previous-state snapshot and reports no
changes or events.

## I2C short reads

`mcp2221_i2c_read_ex()` and `mcp2221_i2c_read_simple()` report
`MCP2221_ERR_I2C_SHORT_READ` when the device completes a read before all
requested bytes have been received.

Applications should handle this as an explicit error rather than treating a
successful but incomplete transfer as valid data.

## Flash errors

Flash operations distinguish read and write failures:

```c
MCP2221_ERR_FLASH_READ
MCP2221_ERR_FLASH_WRITE
MCP2221_ERR_FLASH_PASSWD
```

These flash-specific errors represent failures of the corresponding MCP2221
flash command. Transport, timeout and protocol errors are propagated unchanged.

Higher-level helpers such as `mcp2221_flash_read_info()` and
`mcp2221_flash_save_config()` preserve that layering rather than collapsing all
failures into `MCP2221_ERR_FLASH_READ`.

## Building migrated applications

Using pkg-config:

```sh
cc application.c $(pkg-config --cflags --libs libeasymcp2221)
```

For static linking:

```sh
cc application.c \
  $(pkg-config --cflags --static --libs libeasymcp2221)
```

The pkg-config compiler flags provide the include directory containing the
`libeasymcp2221/` header namespace.

## Migration checklist

When updating an application from 1.x to 2.x:

1. Replace unprefixed include files with `mcp2221_*.h` headers.
2. Add the `libeasymcp2221/` prefix to installed-header includes.
3. Replace legacy type names with `mcp2221_*_t`.
4. Replace deprecated function wrappers with their canonical `mcp2221_*`
   functions.
5. Replace `MCP_ERR_*` with `MCP2221_ERR_*`.
6. Replace unprefixed constants and macros with `MCP2221_*`.
7. Replace `mcp_error_code_to_string()` with
   `mcp2221_error_code_to_string()`.
8. Remove use of the old `mcp_error_t` wrapper API.
9. Check I2C calls for the required `_ex` or `_simple` variant.
10. Verify caller ownership of I2C slave and SMBus context storage.
11. Reorder the first two arguments when replacing
    `mcp2221_create_i2c_slave(dev, slave, ...)` with
    `mcp2221_i2c_slave_init(slave, dev, ...)`.
12. Rebuild with warnings enabled and resolve every removed declaration.
13. Run the application's I2C, GPIO, SRAM, flash and SMBus tests against the
    v2 library.
