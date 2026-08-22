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

## Package releases

Release asset names include both the target userspace and CPU architecture,
while the package metadata inside each `.deb` keeps the normal Debian package
architecture.

Use the [latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest)
for normal installations. Development builds are published in the rolling
[`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot)
prerelease. The matrix below maps common SBC families to the appropriate asset
suffix or snapshot bundle.

## Package selection matrix

Use the table below to choose a package by **userspace** and package
architecture, not by board name alone. The stable link opens the newest
non-prerelease GitHub release; the snapshot link opens the rolling
`v2-snapshot` prerelease.

> [!NOTE]
> GitHub can provide a durable `releases/latest/download/<asset-name>` URL only
> when the asset filename itself is stable. libeasymcp2221 release assets
> currently contain the Debian package version in their filenames, so this
> table links to the durable release pages and shows the matching asset suffix
> or snapshot bundle name to select.

| Board / SBC family | Platform | Distribution / userspace | Latest stable release | Latest development snapshot | Status |
|---|---|---|---|---|---|
| Raspberry Pi 1 / Zero / Zero W / Compute Module 1 | ARMv6 (`armhf` package metadata) | Raspberry Pi OS Bookworm 32-bit | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `rpios-bookworm-armv6` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_rpios-bookworm-armv6.tar.gz` | Official build |
| Raspberry Pi 2/3/4/5, Zero 2 W, CM3/4/5 running 32-bit Bookworm | ARMv7 / `armhf` | Debian/Raspberry Pi OS Bookworm 32-bit | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `debian-bookworm-armhf` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_debian-bookworm-armhf.tar.gz` | Official build |
| Raspberry Pi 3/4/5, Zero 2 W, CM3/4/5 running 64-bit Bookworm | ARM64 / `arm64` | Debian/Raspberry Pi OS Bookworm 64-bit | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `debian-bookworm-arm64` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_debian-bookworm-arm64.tar.gz` | Official build |
| Orange Pi 5 / 5B / 5 Plus, Zero 3, CM4 | ARM64 / `arm64` | Debian Bookworm | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `debian-bookworm-arm64` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_debian-bookworm-arm64.tar.gz` | Package target available; board not individually validated |
| Orange Pi 5 / 5B / 5 Plus, Zero 3, CM4 | ARM64 / `arm64` | Ubuntu 22.04 or 24.04 | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select matching `ubuntu-22.04-arm64` or `ubuntu-24.04-arm64` suffix when attached | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select matching Ubuntu ARM64 bundle | Package target available; board not individually validated |
| Banana Pi BPI-M5 / M7 / M4 Zero | ARM64 / `arm64` | Debian Bookworm or matching Ubuntu release | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select the matching Debian/Ubuntu ARM64 suffix | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select the matching ARM64 bundle | Package target available; board not individually validated |
| FriendlyElec NanoPi R5/R6, NanoPC-T6, CM3588 | ARM64 / `arm64` | Debian Bookworm or matching Ubuntu release | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select the matching Debian/Ubuntu ARM64 suffix | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select the matching ARM64 bundle | Package target available; board not individually validated |
| Khadas VIM3 / VIM4 / Edge2 | ARM64 / `arm64` | Debian Bookworm or matching Ubuntu release | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select the matching Debian/Ubuntu ARM64 suffix | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select the matching ARM64 bundle | Package target available; board not individually validated |
| Libre Computer Le Potato / Sweet Potato / Renegade | ARM64 / `arm64` | Debian Bookworm or matching Ubuntu release | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select the matching Debian/Ubuntu ARM64 suffix | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select the matching ARM64 bundle | Package target available; board not individually validated |
| ODROID-C4 / N2 / N2+ / M1 / M1S | ARM64 / `arm64` | Debian Bookworm or matching Ubuntu release | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select the matching Debian/Ubuntu ARM64 suffix | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select the matching ARM64 bundle | Package target available; board not individually validated |
| Radxa ROCK 4 / ROCK 5, PINE64 ROCKPro64 / Quartz64 | ARM64 / `arm64` | Debian Bookworm or matching Ubuntu release | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select the matching Debian/Ubuntu ARM64 suffix | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — select the matching ARM64 bundle | Package target available; board not individually validated |
| NVIDIA Jetson Nano / TX2 / Xavier on JetPack 4 | ARM64 | Ubuntu 18.04 | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `ubuntu-18.04-arm64` when compatibility assets are attached | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_ubuntu-18.04-arm64.tar.gz` | Compatibility build |
| NVIDIA Xavier / early Orin on JetPack 5 | ARM64 | Ubuntu 20.04 | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `ubuntu-20.04-arm64` when compatibility assets are attached | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_ubuntu-20.04-arm64.tar.gz` | Compatibility build |
| NVIDIA Orin on JetPack 6 and other matching ARM64 SBCs | ARM64 | Ubuntu 22.04 | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `ubuntu-22.04-arm64` when compatibility assets are attached | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_ubuntu-22.04-arm64.tar.gz` | Official snapshot build / compatibility release build |
| Newer generic Ubuntu ARM64 SBC installations | ARM64 | Ubuntu 24.04 | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `ubuntu-24.04-arm64` when compatibility assets are attached | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_ubuntu-24.04-arm64.tar.gz` | Official snapshot build / compatibility release build |
| BeagleV-Ahead and matching RISC-V 64 systems | RISC-V 64 / `riscv64` | Debian Trixie | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — select suffix `debian-trixie-riscv64` when compatibility assets are attached | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — `libeasymcp2221-snapshot_debian-trixie-riscv64.tar.gz` | Compatibility build |
| StarFive VisionFive 2 / 2 Lite | RISC-V 64 / `riscv64` | Debian Trixie only when userspace/ABI matches | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — candidate suffix `debian-trixie-riscv64` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — candidate `libeasymcp2221-snapshot_debian-trixie-riscv64.tar.gz` | Candidate; board not individually validated |
| Milk-V Mars / Pioneer | RISC-V 64 / `riscv64` | Debian Trixie only when userspace/ABI matches | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — candidate suffix `debian-trixie-riscv64` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — candidate `libeasymcp2221-snapshot_debian-trixie-riscv64.tar.gz` | Candidate; board not individually validated |
| Banana Pi BPI-F3 | RISC-V 64 / `riscv64` | Debian Trixie only when userspace/ABI matches | [Latest stable release](https://github.com/tobiasvogel/libeasymcp2221/releases/latest) — candidate suffix `debian-trixie-riscv64` | [`v2-snapshot`](https://github.com/tobiasvogel/libeasymcp2221/releases/tag/v2-snapshot) — candidate `libeasymcp2221-snapshot_debian-trixie-riscv64.tar.gz` | Candidate; board not individually validated |

For stable releases, compatibility assets may be attached after the canonical
release by the separate compatibility-package workflow. If the expected suffix
is not present on the latest stable release, use the matching development
snapshot or build from source.

## Raspberry Pi

| Model | CPU class | Recommended OS/package |
|---|---|---|
| Raspberry Pi 1 Model A / A+ | ARMv6 | Raspberry Pi OS 32-bit, `rpios-bookworm-armv6` |
| Raspberry Pi 1 Model B / B+ | ARMv6 | Raspberry Pi OS 32-bit, `rpios-bookworm-armv6` |
| Raspberry Pi Zero | ARMv6 | Raspberry Pi OS 32-bit, `rpios-bookworm-armv6` |
| Raspberry Pi Zero W / Zero WH | ARMv6 | Raspberry Pi OS 32-bit, `rpios-bookworm-armv6` |
| Compute Module 1 | ARMv6 | Raspberry Pi OS 32-bit, `rpios-bookworm-armv6` |
| Raspberry Pi 2 Model B rev. 1.1 | ARMv7 | Raspberry Pi OS 32-bit, `armhf` |
| Raspberry Pi 2 Model B rev. 1.2 | ARMv8 | `armhf` or `arm64`, depending on OS |
| Raspberry Pi 3 Model A+ | ARMv8 | `armhf` or `arm64`, depending on OS |
| Raspberry Pi 3 Model B / B+ | ARMv8 | `armhf` or `arm64`, depending on OS |
| Raspberry Pi Zero 2 W | ARMv8 | `armhf` or `arm64`, depending on OS |
| Compute Module 3 / 3+ | ARMv8 | `armhf` or `arm64`, depending on OS |
| Raspberry Pi 4 Model B | ARMv8 | Raspberry Pi OS 64-bit, `arm64` |
| Raspberry Pi 400 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` |
| Compute Module 4 / 4S | ARMv8 | Raspberry Pi OS 64-bit, `arm64` |
| Raspberry Pi 5 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` |
| Raspberry Pi 500 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` |
| Compute Module 5 | ARMv8 | Raspberry Pi OS 64-bit, `arm64` |

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

| Model | Architecture | Common userspace | Recommended package |
|---|---|---|---|
| Jetson Nano | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu-18.04-arm64` |
| Jetson Nano 2GB | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu-18.04-arm64` |
| Jetson TX2 | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu-18.04-arm64` |
| Jetson Xavier NX | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu-18.04-arm64` |
| Jetson Xavier NX | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu-20.04-arm64` |
| Jetson AGX Xavier | ARM64 | JetPack 4 / Ubuntu 18.04 | `ubuntu-18.04-arm64` |
| Jetson AGX Xavier | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu-20.04-arm64` |
| Jetson Orin Nano | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu-20.04-arm64` |
| Jetson Orin Nano | ARM64 | JetPack 6 / Ubuntu 22.04 | `ubuntu-22.04-arm64` |
| Jetson Orin NX | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu-20.04-arm64` |
| Jetson Orin NX | ARM64 | JetPack 6 / Ubuntu 22.04 | `ubuntu-22.04-arm64` |
| Jetson AGX Orin | ARM64 | JetPack 5 / Ubuntu 20.04 | `ubuntu-20.04-arm64` |
| Jetson AGX Orin | ARM64 | JetPack 6 / Ubuntu 22.04 | `ubuntu-22.04-arm64` |

Always select the package based on the installed JetPack/Ubuntu version rather
than the Jetson model alone.

For example, after running the compatibility workflow for v2.0.0:

```text
libeasymcp2221-2_2.0.0-1_ubuntu-18.04-arm64.deb
libeasymcp2221-2_2.0.0-1_ubuntu-20.04-arm64.deb
libeasymcp2221-2_2.0.0-1_ubuntu-22.04-arm64.deb
libeasymcp2221-2_2.0.0-1_ubuntu-24.04-arm64.deb
```

## BeagleBoard / BeagleBone

| Model | Architecture | Recommended package |
|---|---|---|
| BeagleBone Black | ARMv7 | Debian Bookworm `armhf` |
| BeagleBone Black Wireless | ARMv7 | Debian Bookworm `armhf` |
| BeagleBone Blue | ARMv7 | Debian Bookworm `armhf` |
| PocketBeagle | ARMv7 | Debian Bookworm `armhf` |
| BeagleBone AI | ARMv7 | Debian Bookworm `armhf` |
| BeagleBone AI-64 | ARM64 | Debian Bookworm `arm64` |
| PocketBeagle 2 | ARM64 | Debian Bookworm `arm64` |
| BeagleV-Ahead | RISC-V 64 | Debian Trixie `riscv64` |

The Debian package must match the Debian release installed on the board. The
Bookworm packages above should not be treated as generic binaries for arbitrary
older Debian images.

## Compatibility notes for other SBCs

The package-selection matrix above is the authoritative board-family overview.
The board vendor itself is not relevant to libeasymcp2221; compatibility is
primarily determined by:

1. CPU instruction-set architecture;
2. Linux distribution and release;
3. libc ABI;
4. availability of `libusb-1.0`.

A board listed in the matrix as having a matching package target has not
necessarily been tested individually. In particular, RISC-V boards should use
the `debian-trixie-riscv64` package only when their installed userspace and ABI
match Debian Trixie.

Boards using Buildroot, OpenWrt/FriendlyWrt, vendor-specific root filesystems,
or another non-Debian/non-Ubuntu userspace should generally be built from
source instead of using one of the pre-built `.deb` packages.

## Installing a package

Runtime-only installation:

```console
sudo apt install ./libeasymcp2221-2_2.0.0-1_debian-bookworm-arm64.deb
```

For software development:

```console
sudo apt install \
  ./libeasymcp2221-2_2.0.0-1_debian-bookworm-arm64.deb \
  ./libeasymcp2221-dev_2.0.0-1_debian-bookworm-arm64.deb
```

For a Jetson Nano running Ubuntu 18.04, use the compatibility package instead:

```console
sudo apt install \
  ./libeasymcp2221-2_2.0.0-1_ubuntu-18.04-arm64.deb \
  ./libeasymcp2221-dev_2.0.0-1_ubuntu-18.04-arm64.deb
```

## Building from source

If no pre-built package matches the system, libeasymcp2221 can also be built
directly:

```console
sudo apt install build-essential cmake pkg-config libusb-1.0-0-dev

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBEASYMCP2221_BUILD_SHARED=ON \
  -DLIBEASYMCP2221_BUILD_STATIC=ON

cmake --build build
sudo cmake --build build --target install
```