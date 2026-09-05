# Hardware tests

These tests exercise libeasymcp2221 against a physical MCP2221 test fixture.
They are disabled by default and are separate from the fake-libusb/unit tests.

## Fixture

The hardware tests expect the EasyMCP2221 reference-style fixture:

- MCP2221 SDA connected to the EEPROM SDA pin.
- MCP2221 SCL connected to the EEPROM SCL pin.
- GP0 connected to SCL through a 1 kOhm resistor.
- GP1 connected to SDA through a 1 kOhm resistor.
- GP2 connected directly to GP3.
- A 24LC256-compatible EEPROM at 7-bit I2C address `0x50` by default.
- EEPROM A0/A1/A2, WP, and VSS connected to GND; VCC connected to the
  fixture supply.

The GP2/GP3 connection is used in both directions:

- GPIO test: GP2 output -> GP3 input.
- Analog test: GP3 DAC2 -> GP2 ADC2.

GP0 and GP1 are used only to inject controlled SCL/SDA-low faults through the
1 kOhm resistors. The tests return all GP pins to GPIO-input safe state during
normal cleanup.

## Configure and build

Enable the hardware tests explicitly:

```sh
cmake -S . -B build \
  -DLIBEASYMCP2221_BUILD_TESTS=ON \
  -DLIBEASYMCP2221_BUILD_HARDWARE_TESTS=ON
cmake --build build
```

Hardware tests link the static library when
`LIBEASYMCP2221_BUILD_STATIC=ON`; otherwise they use the shared-library target.

## Run

Run only hardware tests:

```sh
ctest --test-dir build -L hardware --output-on-failure -V
```

Run one hardware test:

```sh
ctest --test-dir build -R test_hw_persistence --output-on-failure -V
```

Run the complete configured test suite:

```sh
ctest --test-dir build --output-on-failure
```

If no matching MCP2221 is present, hardware tests return CTest skip code 77
instead of failing.

## Device selection

The fixture defaults can be overridden with environment variables:

| Variable | Default | Meaning |
| --- | --- | --- |
| `LIBEASYMCP2221_HW_VID` | `0x04d8` | USB vendor ID |
| `LIBEASYMCP2221_HW_PID` | `0x00dd` | USB product ID |
| `LIBEASYMCP2221_HW_DEVNUM` | `0` | Matching-device index |
| `LIBEASYMCP2221_HW_SERIAL` | unset | USB serial to select |
| `LIBEASYMCP2221_HW_EEPROM_ADDR` | `0x50` | 7-bit EEPROM I2C address |

Numeric values are parsed with C `strtol(..., base=0)`, so decimal and `0x...`
forms are accepted.

## Tests

- `test_hw_device`: opens the selected MCP2221 and validates basic device
  access.
- `test_hw_gpio`: verifies GP2-output to GP3-input loopback.
- `test_hw_i2c_eeprom`: performs deterministic EEPROM write/readback and
  restores the original data.
- `test_hw_analog`: verifies GP3 DAC2 to GP2 ADC2 loopback across fixed voltage
  references.
- `test_hw_i2c_faults`: forces SCL/SDA low through GP0/GP1 and verifies fault
  diagnosis and bus recovery.
- `test_hw_i2c_chunks`: verifies expected NACK handling plus EEPROM transfers
  at the 55..64-byte chunk boundary and restores the original EEPROM page.
- `test_hw_persistence`: saves runtime GPIO configuration to flash, performs a
  real MCP2221 reset, waits for USB re-enumeration, verifies that flash
  settings were loaded into startup SRAM, and restores the original flash
  settings.

## Persistence-test caution

`test_hw_persistence` is intentionally more invasive than the other hardware
tests because it changes flash configuration and issues a real MCP2221 reset.

The test keeps a copy of the original chip/GP flash settings and restores them
after verification. It also makes a bounded recovery attempt if reset
re-enumeration fails. If the device does not return to USB at all, software
cannot restore flash until the MCP2221 is visible again; a physical USB replug
may be required before rerunning the test.

Do not use `test_hw_persistence` as a high-frequency reset stress test. The
normal hardware suite is intended to execute it once per test run.
