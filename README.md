# ESP32 Component Pinout Reference

This document records the pin assignments extracted from the **Pocket_Node** firmware. It is intended as the wiring reference for connecting and testing the Pocket_Node components.

> **Board warning:** Pocket_Node's `platformio.ini` targets `esp32dev` (classic ESP32), not ESP32-S3. The GPIO numbers below are the original Pocket_Node assignments. Do not wire them to an ESP32-S3 until the pin mapping has been checked against the exact S3 board and its flash/PSRAM pin usage.



## Firmware source references

The pin assignments above were read from these Pocket_Node files:

- `src/TFT_eSPI_Setup.h` — TFT display SPI pins
- `src/drivers/sdcard/sdcard.cpp` — SD card HSPI pins
- `src/main.cpp` — I²C bus pins
- `src/drivers/keypad/keypad.cpp` — PCF8575 address and keypad matrix port pins

- `src/apps/messages/messages_config.h` — LoRa SPI and control pins
- `src/drivers/battery/battery.cpp` — battery ADC pin and divider


## Current test project

- Board configured in this repository: `esp32-s3-devkitc-1`
- Framework: Arduino
- Serial Monitor: 115200 baud
- Main source: `src/main.cpp`


ocket Node – Complete component wiring table
S.No.	Component	Component pin / signal	ESP32-S3 GPIO
1	ESP32-S3 N16R8	TFT SCK	GPIO 12
2	ILI9341 TFT	MOSI	GPIO 11
3	ILI9341 TFT	MISO	GPIO 13
4	ILI9341 TFT	CS	GPIO 10
5	ILI9341 TFT	DC	GPIO 9
6	ILI9341 TFT	RESET	GPIO 8
7	microSD module	SCK	GPIO 18
8	microSD module	MOSI	GPIO 17
9	microSD module	MISO	GPIO 16
10	microSD module	CS	GPIO 6
11	RA-02 LoRa SX1278	SCK	GPIO 18
12	RA-02 LoRa SX1278	MOSI	GPIO 17
13	RA-02 LoRa SX1278	MISO	GPIO 16
14	RA-02 LoRa SX1278	NSS / CS	GPIO 15
15	RA-02 LoRa SX1278	RESET	GPIO 14
16	RA-02 LoRa SX1278	DIO0	GPIO 21
17	PCF8575	SDA	GPIO 1
18	PCF8575	SCL	GPIO 2
19	RDA5807M FM	SDA	GPIO 1
20	RDA5807M FM	SCL	GPIO 2
21	NEO-6M GPS	TX	GPIO 39
22	NEO-6M GPS	RX	GPIO 40
23	MAX98357A	BCLK	GPIO 42
24	MAX98357A	LRC / WS	GPIO 41
25	MAX98357A	DIN	GPIO 47
26	Battery voltage divider	ADC output	GPIO 4

Additional component connections
Component	Pin	Connection
PCF8575	VCC	3.3V (verify module compatibility)
PCF8575	GND	GND
RDA5807M	VCC	According to module specification
RDA5807M	GND	GND
NEO-6M	VCC	According to module specification
NEO-6M	GND	GND
MAX98357A	VIN	According to amplifier board specification
MAX98357A	GND	GND
MAX98357A	SPK+ / SPK−	Speaker terminals
RA-02	VCC	Stable 3.3V supply
RA-02	GND	GND
ILI9341	VCC	According to module specification
ILI9341	GND	GND
microSD	VCC	According to module specification
microSD	GND	GND

Shared connections
Bus	Shared components	ESP32-S3 connections
SPI 1	ILI9341 only	GPIO 11, 12, 13
SPI 2	RA-02 + microSD	GPIO 16, 17, 18
I²C	PCF8575 + RDA5807M	GPIO 1, 2
UART	NEO-6M GPS	GPIO 39, 40
I²S	MAX98357A	GPIO 41, 42, 47
ADC	Battery voltage divider	GPIO 4

Important notes:
- GPIO numbers are proposed assignments, not physical header positions.
- LoRa and SD share SPI signals but have separate CS pins.
- TFT touch functionality is excluded.
- RDA5807M analog audio cannot connect directly to the MAX98357A digital I²S input.
- GPS TX connects to ESP32 RX, and GPS RX connects to ESP32 TX.
- All modules must have compatible voltage levels and a common ground.