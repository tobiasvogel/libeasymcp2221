[![CI](https://github.com/tobiasvogel/libeasymcp2221/actions/workflows/ci.yml/badge.svg)](https://github.com/tobiasvogel/libeasymcp2221/actions/workflows/ci.yml)
[![Build v2 Debian snapshots](https://github.com/tobiasvogel/libeasymcp2221/actions/workflows/build-v2-debs.yml/badge.svg)](https://github.com/tobiasvogel/libeasymcp2221/actions/workflows/build-v2-debs.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# libeasymcp2221

A C implementation of the [EasyMCP2221](https://github.com/electronicayciencia/EasyMCP2221) Python module for Microchip MCP2221/MCP2221A USB-to-I2C/GPIO bridges. The project aims for a conceptual 1:1 port from Python to C, while using C-style handles, explicit error codes and function-based APIs instead of Python classes and exceptions.

## Features

- Open/reuse MCP2221 devices by VID/PID, device index or USB serial.
- I2C master read/write operations with explicit transfer kinds and timeout handling.
- Convenience I2C slave and SMBus helpers.
- GPIO read/write, GPIO polling, pin-function configuration and SRAM/flash settings helpers.
- ADC and DAC helpers for raw, normalized and voltage-based values, including
  configurable VDD reference handling.
- USB enumeration attributes for Remote Wake-up capability, self-powered
  declaration and requested USB bus current.
- Shared and static library builds with pkg-config support.

## Documentation

- [`BUILD.md`](BUILD.md) — build, install and packaging notes.
- [`API-Reference.md`](API-Reference.md) — mapping between EasyMCP2221 concepts and the libeasymcp2221 v2 C API.
- [`MIGRATION.md`](MIGRATION.md) — guide for migrating applications from libeasymcp2221 1.x to 2.x.
- [`examples/`](examples/) — small programs demonstrating the public v2 API.

The USB power helpers configure MCP2221 enumeration attributes. They do not
control host USB autosuspend, switch USB port power, or change the physical
power source of the device. See `API-Reference.md` and `examples/usb_power.c`.

## Build

```sh
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

Useful CMake options:

```sh
cmake -S . -B build \
  -DLIBEASYMCP2221_BUILD_SHARED=ON \
  -DLIBEASYMCP2221_BUILD_STATIC=ON \
  -DLIBEASYMCP2221_BUILD_EXAMPLES=ON \
  -DLIBEASYMCP2221_BUILD_TESTS=ON
```

See `BUILD.md` for Debian packaging and installation details.

## Tests

The unit tests are hardware-independent and do not require an attached MCP2221.

```sh
cmake -S . -B build-tests \
  -DLIBEASYMCP2221_BUILD_TESTS=ON \
  -DLIBEASYMCP2221_BUILD_EXAMPLES=OFF

cmake --build build-tests
(cd build-tests && ctest --output-on-failure)
```

## pkg-config

```sh
cc example.c $(pkg-config --cflags --libs libeasymcp2221)
```

For static linking, use:

```sh
cc example.c $(pkg-config --cflags --static --libs libeasymcp2221)
```

## Minimal example

```c
#include <libeasymcp2221/mcp2221.h>
#include <libeasymcp2221/mcp2221_constants.h>

int main(void) {
    mcp2221_t *dev = mcp2221_open_simple(MCP2221_DEV_DEFAULT_VID,
                                          MCP2221_DEV_DEFAULT_PID,
                                          0, NULL, 100000);
    if (!dev)
        return 1;

    mcp2221_close(dev);

    return 0;
}
```
`mcp2221_open_simple()` applies the requested I2C speed during initialization,
so a second `mcp2221_i2c_set_speed()` call is not required.

## Error handling

Most public functions return `MCP2221_ERR_OK` on success or another
`mcp2221_error_code_t` value on failure. Functions that return payload lengths
generally report them through output parameters. Some collection-oriented
functions, such as `mcp2221_gpio_poll_events()`, return a non-negative item
count on success and a negative error code on failure. GPIO read helpers
return an error code and report per-pin state through output arrays.

Use `mcp2221_error_code_to_string()` for human-readable symbolic error names.
Common error codes include `MCP2221_ERR_USB`, `MCP2221_ERR_TIMEOUT`,
`MCP2221_ERR_NOT_ACK`, `MCP2221_ERR_I2C`,
`MCP2221_ERR_I2C_SHORT_READ`, `MCP2221_ERR_FLASH_READ` and
`MCP2221_ERR_FLASH_WRITE`.

## Resource ownership

- `mcp2221_open*()` returns an owned `mcp2221_t *`; release it with `mcp2221_close()`.
- `mcp2221_smbus_init()` borrows `existing_mcp` when one is supplied. In that case, `mcp2221_smbus_close()` does not close the MCP2221 handle.
- If `mcp2221_smbus_init()` opens the MCP2221 handle itself, `mcp2221_smbus_close()` releases it.

## SMBus block size

`MCP2221_I2C_SMBUS_BLOCK_MAX` is 255 to match EasyMCP2221's SMBus compatibility layer: block helper lengths are encoded in one byte, and the public limit describes payload bytes. This is intentionally larger than the classic 32-byte SMBus block limit. Applications that must follow strict SMBus-only semantics should cap block payloads at 32 bytes themselves.

## Keep sentinels

Use `MCP2221_GPIO_KEEP` in `mcp2221_gpio_write_t` fields to preserve an output pin value. Use `MCP2221_CONFIG_KEEP` only in SRAM configuration structures. Both sentinels currently have the value `-1`, but the separate names make the intended API domain explicit.

## Thread safety

`mcp2221_open*()` and `mcp2221_close()` are internally serialized for the global libusb context, reference counter and device catalog. Operations on an already opened `mcp2221_t *` are not serialized by the library; protect shared handles with an application-level mutex when using them from multiple threads.

## API naming

The public v2 API follows one consistent naming scheme:

- functions: `mcp2221_<domain>_<verb>[_object]()`
- types: `mcp2221_<domain>_<name>_t`
- public constants and macros: `MCP2221_<DOMAIN>_<NAME>`

The compatibility aliases and unprefixed public headers provided by the
1.x series were removed in version 2. Applications upgrading from 1.x
should follow `MIGRATION.md`.

## Author
Tobias X. Vogel

## License
MIT License (MIT)

### AI disclaimer
A code review and some implementations after the initial 1.0.0 version were done with the help of OpenAI Codex.
