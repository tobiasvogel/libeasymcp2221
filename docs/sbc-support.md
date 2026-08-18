# Single-board computer support

libeasymcp2221 is a portable C library and can be used on a wide range of
Linux-based single-board computers (SBCs).

Pre-built Debian packages are provided for the most common CPU architectures
and selected Linux distributions.

> [!IMPORTANT]
> Package compatibility depends on both the CPU architecture and the Linux
> distribution/userspace.
>
> For example, an ARM64 binary built for Debian 12 (Bookworm) should not be
> assumed to work on an older Ubuntu installation simply because both systems
> report `aarch64`.

## Determine your system

Check the Debian package architecture:

```console
$ dpkg --print-architecture
arm64
```

Check the CPU architecture:

```console
$ uname -m
aarch64
```

Check the Linux distribution:

```console
$ cat /etc/os-release
```

## Release v2.0.0

The canonical v2.0.0 release packages are:

| Target | Runtime package | Development package |
|---|---|---|
| Debian 12 / ARM64 | [arm64 runtime](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-2_2.0.0-1_arm64.deb) | [arm64 development](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-dev_2.0.0-1_arm64.deb) |
| Debian 12 / ARMv7 | [armhf runtime](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-2_2.0.0-1_armhf.deb) | [armhf development](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-dev_2.0.0-1_armhf.deb) |
| Raspberry Pi OS / ARMv6 | [ARMv6 runtime](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-2_2.0.0-1_rpios-armv6.deb) | [ARMv6 development](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-dev_2.0.0-1_rpios-armv6.deb) |
| Debian 12 / AMD64 | [amd64 runtime](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-2_2.0.0-1_amd64.deb) | [amd64 development](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-dev_2.0.0-1_amd64.deb) |
| Debian 12 / i386 | [i386 runtime](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-2_2.0.0-1_i386.deb) | [i386 development](https://github.com/tobiasvogel/libeasymcp2221/releases/download/v2.0.0/libeasymcp2221-dev_2.0.0-1_i386.deb) |

The optional SBC compatibility workflow adds:

| Target | Asset suffix |
|---|---|
| Ubuntu 18.04 / ARM64 | `ubuntu18.04-arm64` |
| Ubuntu 20.04 / ARM64 | `ubuntu20.04-arm64` |
| Ubuntu 22.04 / ARM64 | `ubuntu22.04-arm64` |
| Ubuntu 24.04 / ARM64 | `ubuntu24.04-arm64` |
| Debian Trixie / RISC-V 64 | `debian-trixie-riscv64` |

## Raspberry Pi

| Model | CPU class | Recommended OS/package | Status |
|---|---|---|---|
| Raspberry Pi 1 Model A / A+ | ARMv6 | Raspberry Pi OS 32-bit, `rpios-armv6` | ✅ |
| Raspberry Pi 1 Model B / B+ | ARMv6 | Raspberry Pi OS 32-bit, `rpios-armv6` | ✅ |
| Raspberry Pi Zero | ARMv6 | Raspberry Pi OS 32-bit, `rpios-armv6` | ✅ |
| Raspberry Pi Zero W / Zero WH | ARMv6 | Raspberry Pi OS 32-bit, `rpios-armv6` | ✅ |
| Compute Module 1 | ARMv6 | Raspberry Pi OS 32-bit, `rpios-armv6` | ✅ |
| Raspberry Pi 2 Model B rev. 1.1 | ARMv7 | Raspberry Pi OS 32-bit, `armhf` | ✅ |
| Raspberry Pi 2 Model B rev. 1.2 | ARMv8 | `armhf` or `arm64`, depending on OS | ✅ |
| Raspberry Pi 3 Model A+ | ARMv8 | `armhf` or `arm64`, depending on OS | ✅ |
| Raspberry Pi 3 Model B / B+ | ARMv8 | `armhf` or `arm64`, depending on OS | ✅ |
| Raspberry Pi Zero 2 W | ARMv8 | `armhf` or `arm64`, depending on OS | ✅ |
| Compute Module 3 / 3+ | ARMv8 | `armhf` or `arm64`, depending on OS | ✅ |
| Raspberry Pi 4 Model B | ARMv8 | Raspberry Pi OS 64-bit, `arm64` | ✅ |
| Raspberry Pi 400 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` | ✅ |
| Compute Module 4 / 4S | ARMv8 | Raspberry Pi OS 64-bit, `arm64` | ✅ |
| Raspberry Pi 5 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` | ✅ |
| Raspberry Pi 500 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` | ✅ |
| Compute Module 5 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` | ✅ |

For Raspberry Pi OS, prefer the package matching the userspace architecture
reported by:

```console
dpkg --print-architecture
```

Do not select the package solely from the capabilities of the CPU. A 64-bit
CPU can still be running a 32-bit Raspberry Pi OS installation.

## NVIDIA Jetson

Jetson systems require special attention because JetPack releases use specific
Ubuntu userspaces.

| Model | Architecture | Common userspace | Recommended package | Status |
|---|---|---|---|---|
| Jetson Nano | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu18.04-arm64` | ✅ compatibility workflow |
| Jetson Nano 2GB | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu18.04-arm64` | ✅ compatibility workflow |
| Jetson TX2 | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu18.04-arm64` | ✅ compatibility workflow |
| Jetson Xavier NX | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu18.04-arm64` | ✅ compatibility workflow |
| Jetson Xavier NX | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu20.04-arm64` | ✅ compatibility workflow |
| Jetson AGX Xavier | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu18.04-arm64` | ✅ compatibility workflow |
| Jetson AGX Xavier | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu20.04-arm64` | ✅ compatibility workflow |
| Jetson Orin Nano | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu20.04-arm64` | ✅ compatibility workflow |
| Jetson Orin Nano | ARM64 | JetPack 6 / Ubuntu 22.04 | `ubuntu22.04-arm64` | ✅ compatibility workflow |
| Jetson Orin NX | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu20.04-arm64` | ✅ compatibility workflow |
| Jetson Orin NX | ARM64 | JetPack 6 / Ubuntu 22.04 | `ubuntu22.04-arm64` | ✅ compatibility workflow |
| Jetson AGX Orin | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu20.04-arm64` | ✅ compatibility workflow |
| Jetson AGX Orin | ARM64 | JetPack 6 / Ubuntu 22.04 | `ubuntu22.04-arm64` | ✅ compatibility workflow |

Always select the package based on the installed JetPack/Ubuntu version rather
than the Jetson model alone.

For example, after running the compatibility workflow for v2.0.0:

```text
libeasymcp2221-2_2.0.0-1_ubuntu18.04-arm64.deb
libeasymcp2221-2_2.0.0-1_ubuntu20.04-arm64.deb
libeasymcp2221-2_2.0.0-1_ubuntu22.04-arm64.deb
libeasymcp2221-2_2.0.0-1_ubuntu24.04-arm64.deb
```

## BeagleBoard / BeagleBone

| Model | Architecture | Recommended package | Status |
|---|---|---|---|
| BeagleBone Black | ARMv7 | Debian Bookworm `armhf` | ✅ |
| BeagleBone Black Wireless | ARMv7 | Debian Bookworm `armhf` | ✅ |
| BeagleBone Blue | ARMv7 | Debian Bookworm `armhf` | ✅ |
| PocketBeagle | ARMv7 | Debian Bookworm `armhf` | ✅ |
| BeagleBone AI | ARMv7 | Debian Bookworm `armhf` | ✅ |
| BeagleBone AI-64 | ARM64 | Debian Bookworm `arm64` | ✅ |
| PocketBeagle 2 | ARM64 | Debian Bookworm `arm64` | ✅ |
| BeagleV-Ahead | RISC-V 64 | Debian Trixie `riscv64` | ✅ compatibility workflow |

The Debian package must match the Debian release installed on the board. The
Bookworm packages above should not be treated as generic binaries for arbitrary
older Debian images.

## Other common ARM Linux SBCs

The library itself is not tied to Raspberry Pi, Jetson, or BeagleBoard
hardware. Other Linux SBCs can use the same packages when both architecture
and userspace match.

Examples include:

| Board | Architecture | Typical package choice |
|---|---|---|
| Orange Pi 5 / 5B / 5 Plus | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| Orange Pi Zero 3 | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| ODROID-C4 | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| ODROID-N2 / N2+ | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| ODROID-XU4 | ARMv7 | Debian Bookworm `armhf` when using Debian 12 |
| ROCK 5 Model B | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| ROCK 4 series | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| ROCKPro64 | ARM64 | Debian Bookworm `arm64` or matching Ubuntu ARM64 package |
| Pine64 / Pine A64 | ARM64 | Debian Bookworm `arm64` when using Debian 12 |

The board vendor is not relevant to libeasymcp2221 itself. Compatibility is
primarily determined by:

1. CPU instruction-set architecture;
2. Linux distribution and release;
3. libc ABI;
4. availability of `libusb-1.0`.

## Installing a package

Runtime-only installation:

```console
sudo apt install ./libeasymcp2221-2_2.0.0-1_arm64.deb
```

For software development:

```console
