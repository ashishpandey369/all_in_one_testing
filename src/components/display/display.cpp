#include "display.h"

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ESP32-S3 N16R8 to ILI9341 wiring.
namespace
{
constexpr int TFT_SCK  = 12;
constexpr int TFT_MOSI = 11;
constexpr int TFT_MISO = 13;
constexpr int TFT_CS   = 10;
constexpr int TFT_DC   = 9;
constexpr int TFT_RST  = 8;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
}

void displayInit()
{
    // Start SPI with the GPIO assignments specified for this display.
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);

    tft.begin();
    tft.setRotation(1); // Landscape orientation.
    tft.fillScreen(ILI9341_BLACK);
}

void displayShowHi()
{
    const char message[] = "HI";

    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(4);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t textWidth = 0;
    uint16_t textHeight = 0;

    tft.getTextBounds(message, 0, 0, &x1, &y1, &textWidth, &textHeight);

    const int16_t x = (tft.width() - textWidth) / 2 - x1;
    const int16_t y = (tft.height() - textHeight) / 2 - y1;

    tft.setCursor(x, y);
    tft.print(message);
}
