# libeasymcp2221 v2

libeasymcp2221 is a C library for Microchip MCP2221/MCP2221A USB-to-I2C/GPIO
bridges. Version 2 provides a C-style API based on explicit device handles,
error codes, and function-based interfaces.

## API reference

The API reference is generated from the public headers in `include/`. The core
device and I2C master API is declared in `mcp2221.h`; additional headers cover
I2C slave and SMBus helpers, GPIO and pin configuration, SRAM and flash
settings, ADC/DAC support, USB attributes, and error handling.

## Resource ownership

Functions in the `mcp2221_open*()` family return an owned device handle through
an output parameter. Release that handle with `mcp2221_close()`.

Higher-level helper objects document their ownership rules in their respective
public headers.

## Thread safety

Opening and closing devices is internally serialized for the library's global
libusb state. Operations on an already opened `mcp2221_t *` are not serialized
by the library; applications sharing a handle between threads must provide
their own synchronization.

## Further documentation

- `README.md` provides an overview and a minimal example.
- `BUILD.md` describes building, installing, and packaging the library.
- `MIGRATION.md` describes migration from libeasymcp2221 1.x to 2.x.
- `API-Reference.md` maps EasyMCP2221 concepts to the libeasymcp2221 v2 C API.
- `examples/` contains small programs using the public v2 API.
