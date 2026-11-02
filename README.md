# ESP32 Component Pinout Reference

This document records the pin assignments extracted from the **Pocket_Node** firmware. It is intended as the wiring reference for connecting and testing the Pocket_Node components.

> **Board warning:** Pocket_Node's `platformio.ini` targets `esp32dev` (classic ESP32), not ESP32-S3. The GPIO numbers below are the original Pocket_Node assignments. Do not wire them to an ESP32-S3 until the pin mapping has been checked against the exact S3 board and its flash/PSRAM pin usage.

## 1. TFT Display — ILI9341 (VSPI)

| ESP32 GPIO | Display pin |
|---|---|
| GPIO 19 | MISO |
| GPIO 23 | MOSI |
| GPIO 18 | SCLK / SCK |
| GPIO 5 | CS |
| GPIO 2 | DC |
| GPIO 4 | RST |
| 3.3V | VCC / logic supply (check display board specification) |
| GND | GND |

## 2. microSD Card Module — HSPI

| ESP32 GPIO | SD module pin |
|---|---|
| GPIO 32 | MISO |
| GPIO 13 | MOSI |
| GPIO 14 | SCK / CLK |
| GPIO 33 | CS |
| 3.3V | VCC (check module specification) |
| GND | GND |

## 3. PCF8575 I/O Expander — Keypad I²C

| ESP32 GPIO | PCF8575 pin |
|---|---|
| GPIO 21 | SDA |
| GPIO 22 | SCL |
| 3.3V | VCC (recommended for direct ESP32 logic compatibility) |
| GND | GND |

The PCF8575 I²C address in the Pocket_Node code is `0x20` (normally A0, A1 and A2 LOW).

### Keypad matrix connections on PCF8575

These are **PCF8575 port pins**, not ESP32 GPIO pins.

| Keypad signal | PCF8575 pin |
|---|---|
| Row 1 (ROW_PINS[0]) | P3 |
| Row 2 (ROW_PINS[1]) | P11 |
| Row 3 (ROW_PINS[2]) | P5 |
| Row 4 (ROW_PINS[3]) | P6 |
| Row 5 (ROW_PINS[4]) | P8 |
| Row 6 (ROW_PINS[5]) | P2 |
| Row 7 (ROW_PINS[6]) | P13 |
| Row 8 (ROW_PINS[7]) | P14 |
| Column 1 (COL_PINS[0]) | P10 |
| Column 2 (COL_PINS[1]) | P7 |
| Column 3 (COL_PINS[2]) | P0 |
| Column 4 (COL_PINS[3]) | P1 |
| Column 5 (COL_PINS[4]) | P9 |
| Column 6 (COL_PINS[5]) | P4 |
| Column 7 (COL_PINS[6]) | P12 |
| Not assigned in keypad arrays | P15 |

## 4. RDA5807M FM Radio Module — I²C

| ESP32 GPIO | Radio module pin |
|---|---|
| GPIO 21 | SDA |
| GPIO 22 | SCL |
| 3.3V | VCC (check module specification) |
| GND | GND |
| Antenna | ANT |

The radio shares the same I²C bus as the PCF8575. The firmware probes radio addresses `0x10` and `0x11`; do not give the radio a separate set of SDA/SCL GPIOs.

## 5. LoRa Module — SPI

The LoRa pin assignments are defined in `src/apps/messages/messages_config.h`. The LoRa module shares the display's VSPI clock/data lines.

| ESP32 GPIO | LoRa module pin |
|---|---|
| GPIO 18 | SCK |
| GPIO 19 | MISO |
| GPIO 23 | MOSI |
| GPIO 26 | NSS / CS |
| GPIO 27 | RESET / RST |
| GPIO 16 | DIO0 |
| 3.3V | VCC (use a 3.3V-compatible LoRa module) |
| GND | GND |

The configured frequency is 433 MHz in the current source. Confirm that the module and antenna are designed for the permitted frequency band in your region before transmitting.

## 6. Battery Voltage Measurement

| ESP32 GPIO | Battery circuit connection |
|---|---|
| GPIO 35 | Voltage-divider output / ADC input |
| GND | Battery negative / circuit ground |

The source code assumes a 100 kΩ / 100 kΩ divider (ratio 2:1). **Do not connect a Li-ion cell directly to GPIO 35.** The ADC input must remain within the ESP32's allowed voltage range.

## 7. SIM Detection

| ESP32 GPIO | SIM detection circuit |
|---|---|
| GPIO 34 | SIM_DET output |
| GND | Common ground |

GPIO 34 is configured as `INPUT_PULLUP` in the source. On the classic ESP32, GPIO 34 is input-only and does not have an internal pull-up. Use an external pull-up resistor to 3.3V if the detection circuit requires one.

---

## 8. ESP32 GPIOs Already Assigned in Pocket_Node

The following GPIOs are used by the source firmware:

`GPIO 2, 4, 5, 13, 14, 16, 18, 19, 21, 22, 23, 26, 27, 32, 33, 34, 35`

Shared buses are intentional:
- GPIO 18 / 19 / 23 are shared by the TFT display and LoRa SPI.
- GPIO 21 / 22 are shared by the PCF8575 keypad expander and RDA5807M radio I²C bus.

## 9. Not Connected / Available GPIOs — Classic ESP32-WROOM DevKit

Based on the pins used above and a common classic ESP32-WROOM DevKit pinout, these GPIOs are not assigned by the current Pocket_Node source:

| Available ESP32 GPIO | Notes |
|---|---|
| GPIO 0 | Boot-strapping pin; use carefully |
| GPIO 1 | UART0 TX; used for serial output/programming |
| GPIO 3 | UART0 RX; used for serial programming |
| GPIO 12 | Boot-strapping pin; avoid external circuits that force an incorrect boot level |
| GPIO 15 | Boot-strapping pin; use carefully |
| GPIO 17 | Available general-purpose GPIO on many WROOM DevKit boards |
| GPIO 25 | Available general-purpose GPIO on many WROOM DevKit boards |
| GPIO 36 | Input-only; no internal pull-up or pull-down |
| GPIO 39 | Input-only; no internal pull-up or pull-down |

### Pins to avoid or verify

- GPIO 6–11 are normally connected to the module's SPI flash and should not be used as ordinary GPIO.
- GPIO 34–39 are input-only on the classic ESP32. GPIO 34 and 35 are already assigned above.
- GPIO 0, 2, 5, 12 and 15 are boot-strapping pins; attached hardware can prevent normal boot if it drives the wrong level.
- GPIO 1 and 3 are used by the USB-to-UART serial interface on common DevKit boards.
- The exact pins exposed depend on the ESP32 module and development board.

**Important for the current ESP32-S3 keypad test:** the test sketch in this repository currently uses **ESP32-S3 GPIO 8 (SDA) and GPIO 9 (SCL)**. That is separate from the original Pocket_Node classic ESP32 bus assignment of GPIO 21 (SDA) and GPIO 22 (SCL). Keep the wiring matched to the board and firmware you are actually flashing.

---

## Firmware source references

The pin assignments above were read from these Pocket_Node files:

- `src/TFT_eSPI_Setup.h` — TFT display SPI pins
- `src/drivers/sdcard/sdcard.cpp` — SD card HSPI pins
- `src/main.cpp` — I²C bus pins
- `src/drivers/keypad/keypad.cpp` — PCF8575 address and keypad matrix port pins
- `src/apps/radio/radio.cpp` — RDA5807M I²C detection
- `src/apps/messages/messages_config.h` — LoRa SPI and control pins
- `src/drivers/battery/battery.cpp` — battery ADC pin and divider
- `src/drivers/sim/sim.cpp` — SIM detection pin

## Current test project

- Board configured in this repository: `esp32-s3-devkitc-1`
- Framework: Arduino
- Serial Monitor: 115200 baud
- Main source: `src/main.cpp`
