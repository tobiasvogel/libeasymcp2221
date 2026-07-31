# Build Instructions

## Requirements

* C compiler with C99 support
* CMake
* pkg-config
* libusb-1.0 development headers

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

At least one of `LIBEASYMCP2221_BUILD_SHARED` or
`LIBEASYMCP2221_BUILD_STATIC` must be enabled.

## Install

```sh
sudo cmake --install build
```

The install target installs the public headers under
`<prefix>/include/libeasymcp2221`, the selected library artifacts, pkg-config
metadata and the `libeasymcp2221(3)` manual page. When examples are enabled,
their source files are installed as documentation examples.

Installed applications should include public headers using the
`libeasymcp2221/` prefix, for example:

```c
#include <libeasymcp2221/mcp2221.h>
#include <libeasymcp2221/mcp2221_constants.h>
```

For normal system-wide installations, the CMake install script runs
`ldconfig` automatically. When installing into a staging directory or a
custom prefix, update the runtime linker configuration as appropriate for
that environment.

## pkg-config

```sh
pkg-config --cflags --libs libeasymcp2221
```

For static linking:

```sh
pkg-config --cflags --static --libs libeasymcp2221
```

A complete compile command can look like this:

```sh
cc example.c $(pkg-config --cflags --libs libeasymcp2221)
```

## Examples

Examples are built when `LIBEASYMCP2221_BUILD_EXAMPLES=ON`:

```sh
cmake -S . -B build -DLIBEASYMCP2221_BUILD_EXAMPLES=ON
cmake --build build
```

The examples use the public v2 API.

## udev rule

The udev rule is not installed by default. Enable it for non-Debian
system-wide installations with:

```sh
cmake -S . -B build -DLIBEASYMCP2221_INSTALL_UDEV_RULE=ON
cmake --build build
sudo cmake --install build
```

This option is intended for Linux installations that need non-root access to
MCP2221 devices.

## Debian package

```sh
dpkg-buildpackage -us -uc
```
