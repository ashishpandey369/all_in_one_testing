#include "sd_card.h"

#include <SPI.h>
#include <SD.h>

// SD card pins for ESP32-S3.
namespace
{
constexpr int SD_SCK  = 18;
constexpr int SD_CS   = 6;
constexpr int SD_MOSI = 17;
constexpr int SD_MISO = 16;

// Use a separate SPI peripheral so the SD card does not
// change the SPI pin configuration used by the TFT display.
SPIClass sdSPI(HSPI);
}

bool sdCardInit()
{
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (SD.begin(SD_CS, sdSPI, 1000000))
    {
        Serial.println("SUCCESS!");
        Serial.println("SD Card Communication Working!");
        return true;
    }

    Serial.println("FAILED!");
    Serial.println("SD Card Communication Not Working!");
    return false;
}
