[![build-debs](https://github.com/tobiasvogel/libeasymcp2221/actions/workflows/deb.yml/badge.svg)](https://github.com/tobiasvogel/libeasymcp2221/actions/workflows/deb.yml) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# libeasymcp2221

A C implementation of the [EasyMCP2221](https://github.com/electronicayciencia/EasyMCP2221) Python module for Microchip MCP2221/MCP2221A USB-to-I2C/GPIO bridges. The project aims for a conceptual 1:1 port from Python to C, while using C-style handles, explicit error codes and function-based APIs instead of Python classes and exceptions.

## Features

- Open/reuse MCP2221 devices by VID/PID, device index or USB serial.
- I2C master read/write operations with explicit transfer kinds and timeout handling.
- Convenience I2C slave and SMBus helpers.
- GPIO read/write, GPIO polling, pin-function configuration and SRAM/flash settings helpers.
- ADC, DAC, clock and interrupt-on-change helpers.
- Shared and static library builds with pkg-config support.

## Documentation

- `BUILD.md` — build, install and packaging notes.
- `API-Reference.md` — Python EasyMCP2221 to C API mapping.
- `MIGRATION.md` — migration guide for the 1.1 API naming cleanup.
- `examples/` — small example programs using the preferred API names.

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
  -DLIBEASYMCP2221_BUILD_EXAMPLES=ON
```

See `BUILD.md` for Debian packaging and installation details.

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
#include <libeasymcp2221/constants.h>

int main(void) {
    mcp2221_t *dev = mcp2221_open_simple(MCP2221_DEV_DEFAULT_VID,
                                          MCP2221_DEV_DEFAULT_PID,
                                          0, NULL, 100000);
    if (!dev)
        return 1;

    mcp_err_t err = mcp2221_i2c_set_speed(dev, 100000);
    mcp2221_close(dev);

    return err == MCP_ERR_OK ? 0 : 1;
}
```

## Error handling

Most functions return `MCP_ERR_OK` on success or another `mcp_err_t` value on failure. Functions that return data lengths report them through output parameters.

Use `mcp_error_code_to_string()` for human-readable error names. Common error codes include `MCP_ERR_USB`, `MCP_ERR_TIMEOUT`, `MCP_ERR_NOT_ACK`, `MCP_ERR_I2C`, `MCP_ERR_I2C_SHORT_READ`, `MCP_ERR_FLASH_READ` and `MCP_ERR_FLASH_WRITE`.

## Resource ownership

- `mcp2221_open*()` returns an owned `mcp2221_t *`; release it with `mcp2221_close()`.
- `mcp2221_smbus_init()` borrows `existing_mcp` when one is supplied. In that case, `mcp2221_smbus_close()` does not close the MCP2221 handle.
- If `mcp2221_smbus_init()` opens the MCP2221 handle itself, `mcp2221_smbus_close()` releases it.

## SMBus block size

`MCP2221_I2C_SMBUS_BLOCK_MAX` is 255 to match EasyMCP2221's SMBus compatibility layer: block helper lengths are encoded in one byte, and the public limit describes payload bytes. This is intentionally larger than the classic 32-byte SMBus block limit. Applications that must follow strict SMBus-only semantics should cap block payloads at 32 bytes themselves.

## Thread safety

`mcp2221_open*()` and `mcp2221_close()` are internally serialized for the global libusb context, reference counter and device catalog. Operations on an already opened `mcp2221_t *` are not serialized by the library; protect shared handles with an application-level mutex when using them from multiple threads.

## API naming and deprecation policy

Starting with the 1.1 API cleanup, new public names follow one consistent scheme:

- functions: `mcp2221_<domain>_<verb>[_object]()`
- types: `mcp2221_<domain>_<name>_t`
- public constants/macros: `MCP2221_<DOMAIN>_<NAME>`

Older names remain available as compatibility aliases throughout the 1.x series. They are marked with `MCP2221_DEPRECATED("use ...")` where compiler-portable deprecation is possible and may be removed in the next major version, currently planned as 2.0.

Applications should migrate to the `mcp2221_*` naming scheme. During migration, define `MCP2221_NO_DEPRECATED_WARNINGS` before including libeasymcp2221 headers to temporarily suppress compatibility warnings. New code, examples and documentation should use only the preferred names. See `MIGRATION.md` and `API-Reference.md` for the mapping.

## Author
Tobias X. Vogel

## License
MIT License (MIT)

### AI disclaimer
A code review and some implementations after the initial 1.0.0 version were done with the help of OpenAI Codex.
