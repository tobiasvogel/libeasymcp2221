# Build Instructions

## Requirements

* C compiler with C99 support
* CMake
* pkg-config (POSIX and MinGW builds)
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
  -DLIBEASYMCP2221_BUILD_DOCS=ON \
  -DLIBEASYMCP2221_INSTALL_UDEV_RULE=OFF
```

At least one of `LIBEASYMCP2221_BUILD_SHARED` or
`LIBEASYMCP2221_BUILD_STATIC` must be enabled.

## Windows with native MSVC

Native x64 MSVC builds are an officially tested Windows build path. Use
Visual Studio 2022 or newer and install libusb with vcpkg:

```powershell
vcpkg install libusb:x64-windows
```

Configure through the vcpkg CMake toolchain:

```powershell
cmake -S . -B build-msvc -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_INSTALLATION_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DLIBEASYMCP2221_BUILD_SHARED=ON `
  -DLIBEASYMCP2221_BUILD_STATIC=ON

cmake --build build-msvc --config Release
```

MSVC does not provide a C99 compiler mode, so CMake enables `/std:c11` for
MSVC while the project source/API baseline remains C99-compatible.

When both library variants are built, the native MSVC artifacts are:

```text
libeasymcp2221.dll
libeasymcp2221.lib
libeasymcp2221_static.lib
```

`libeasymcp2221.lib` is the import library for the shared DLL;
`libeasymcp2221_static.lib` is the static library. The distinct static name
avoids the `.lib` filename collision that would otherwise occur under MSVC.

## API documentation

API documentation is optional and requires Doxygen plus Graphviz `dot`.

On Debian/Ubuntu:

```sh
sudo apt install doxygen graphviz
```

Configure a documentation build with:

```sh
cmake -S . -B build-docs \
  -DLIBEASYMCP2221_BUILD_DOCS=ON \
  -DLIBEASYMCP2221_BUILD_EXAMPLES=OFF \
  -DLIBEASYMCP2221_BUILD_TESTS=OFF

cmake --build build-docs --target docs
```

The generated HTML documentation is written below
`build-docs/docs/html/`.

## Install

```sh
sudo cmake --install build
```

The install target installs the public headers under
`<prefix>/include/libeasymcp2221`, the selected library artifacts and the
`libeasymcp2221(3)` manual page. POSIX and MinGW installs also include
pkg-config metadata. When examples are enabled, their source files are
installed as documentation examples.

Installed applications should include public headers using the
`libeasymcp2221/` prefix, for example:

```c
#include <libeasymcp2221/mcp2221.h>
#include <libeasymcp2221/mcp2221_constants.h>
```

The CMake install script does not run `ldconfig` or reload udev rules.
System package managers and administrators are responsible for updating
runtime linker and udev state when required. This keeps `DESTDIR`, staging
and custom-prefix installs from modifying the host system.

## pkg-config

pkg-config metadata is generated and installed for POSIX and MinGW builds.
Native MSVC builds use the `.lib` artifacts documented above and do not
install a `.pc` file.

```sh
pkg-config --cflags --libs libeasymcp2221
```

For a static consumer, `pkg-config --static` reports the private dependencies
needed by `libeasymcp2221.a`:

```sh
pkg-config --cflags --static --libs libeasymcp2221
```

It does not force the linker to select `libeasymcp2221.a` when the shared
library is also available. Select the archive explicitly (or otherwise switch
the linker to static lookup for libeasymcp2221) when the library itself must
be linked statically.

A complete shared-library compile command can look like this:

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
