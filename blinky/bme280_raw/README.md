# Part 8: BME280 temperature

The application uses direct I2C register access, not Zephyr's BME280 sensor driver.
The register definitions and temperature compensation follow the
[Bosch BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf),
sections 4.2 and 5.4.

## Hardware configuration

The board overlay selects I2C1 at 100 kHz, with SCL on P1.14 and SDA on P1.15.
The shared `boards/bme280-node.dtsi` defines `bme5180` at address `0x77`.
Use the lab's BME280 breakout with a common ground and the appropriate power
connection; confirm wiring before interpreting a failed sensor read.

The application starts two seconds after reset. It checks chip ID `0x60`, resets
the sensor, reads its factory temperature calibration, and prints a compensated
temperature every two seconds. Each reading triggers a forced measurement with
temperature oversampling x1. Temperature is stored as an integer in hundredths
of a degree, so floating-point printf support is unnecessary.

TF-M UART1 logging is disabled because UART1 and I2C1 share a peripheral instance.
The application still logs through UART0 (most recently COM20, 115200 baud).

From `blinky` in the nRF Connect terminal:

```powershell
west build -d build
if ($LASTEXITCODE -eq 0) { west flash -d build }
```

For part 8.1, capture real temperature output after connecting the sensor.
An initialization-error message does not demonstrate a successful measurement.

## Laptop tests for part 8.2

```powershell
$env:QEMU_BIN_PATH = "C:\Program Files\qemu"
west twister -T tests/BME280_UNIT_TEST -p qemu_cortex_m3 --short-build-path --outdir twister-out-bme280 --inline-logs -v
```

The QEMU tests share the sensor node with the real board but replace the Nordic
controller with a mock I2C controller backed by a register array. They exercise
the real register-access code, including calibration and read/write failures.
The devicetree tests check the shared node's enabled state, bus type, address,
and test-bus speed. They do not validate physical wiring, Nordic pin routing,
or the accuracy of a real sensor. The board build separately checks that the
hardware overlay compiles.

Commit source files and report screenshots; generated build and Twister outputs
are ignored by Git.
