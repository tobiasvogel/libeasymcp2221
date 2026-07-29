# Build Instructions

## Requirements

- C compiler with C99 support
- CMake
- pkg-config
- libusb-1.0 development headers

On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake pkg-config libusb-1.0-0-dev
```

## Configure and build

```sh
cmake -S . -B build
cmake --build build
```

Useful options:

```sh
cmake -S . -B build \
  -DLIBEASYMCP2221_BUILD_SHARED=ON \
  -DLIBEASYMCP2221_BUILD_STATIC=ON \
  -DLIBEASYMCP2221_BUILD_EXAMPLES=ON \
  -DLIBEASYMCP2221_INSTALL_UDEV_RULE=OFF
```

At least one of `LIBEASYMCP2221_BUILD_SHARED` or `LIBEASYMCP2221_BUILD_STATIC` must be enabled.

## Install

```sh
sudo cmake --install build
sudo ldconfig
```

The install target installs headers under the configured include directory, the library artifacts, pkg-config metadata and project documentation.

## pkg-config

```sh
pkg-config --cflags --libs libeasymcp2221
```

For static linking:

```sh
pkg-config --cflags --static --libs libeasymcp2221
```

## Examples

Examples are built when `LIBEASYMCP2221_BUILD_EXAMPLES=ON`:

```sh
cmake -S . -B build -DLIBEASYMCP2221_BUILD_EXAMPLES=ON
cmake --build build
```

The examples use the preferred `mcp2221_*` API names and `MCP2221_*` macro names.

## Debian package

```sh
dpkg-buildpackage -us -uc
```

The Debian package includes `README.md`, `BUILD.md`, `API-Reference.md`, `MIGRATION.md` and `LICENSE` as package documentation.
