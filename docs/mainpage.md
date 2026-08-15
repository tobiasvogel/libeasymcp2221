# libeasymcp2221 v2

libeasymcp2221 is a C library for Microchip MCP2221/MCP2221A USB-to-I2C/GPIO
bridges. Version 2 provides a C-style API based on explicit device handles,
error codes, and function-based interfaces.

## Quick start

Installed headers use the `libeasymcp2221/` include prefix. The umbrella header
provides the complete supported public API:

```c
#include <libeasymcp2221/libeasymcp2221.h>

int main(void) {
    mcp2221_t *dev = NULL;

    mcp2221_error_code_t err =
        mcp2221_open_simple(MCP2221_DEV_DEFAULT_VID,
                            MCP2221_DEV_DEFAULT_PID,
                            0, NULL, 100000, &dev);
    if (err != MCP2221_ERR_OK)
        return 1;

    mcp2221_close(dev);
    return 0;
}
```

Most operations return `MCP2221_ERR_OK` on success or another
`mcp2221_error_code_t` value on failure. Each successful
`mcp2221_open*()` call acquires one device reference and must be matched by one
`mcp2221_close()` call.

## API reference

The API reference is generated from the public headers in `include/`. The core
device and I2C master API is declared in `mcp2221.h`; additional headers cover
I2C slave and SMBus helpers, GPIO and pin configuration, SRAM and flash
settings, ADC/DAC support, USB attributes, and error handling.

## Resource ownership

Each successful `mcp2221_open*()` call acquires one device reference and must
be matched by one `mcp2221_close()` call. A matching device may reuse an
already open underlying handle internally.

Higher-level helper objects document their ownership rules in their respective
public headers.

## Thread safety

Opening and closing devices is internally serialized for the library's global
libusb state. Operations on an already opened `mcp2221_t *` are not serialized
by the library; applications sharing a handle between threads must provide
their own synchronization.

## Further documentation

The generated documentation also includes:

- `BUILD.md` for building, installing, packaging, and generating the API docs.
- `MIGRATION.md` for migration from libeasymcp2221 1.x to 2.x.
- `API-Reference.md` for the EasyMCP2221-to-libeasymcp2221 concept mapping.

`README.md` remains the project overview on GitHub, and `examples/` contains
small programs using the public v2 API.
