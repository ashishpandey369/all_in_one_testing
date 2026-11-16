#include <SPI.h>
#include <SD.h>

#define SD_CS 10
#define SD_SCK 12
#define SD_MOSI 11
#define SD_MISO 13

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("ESP32-S3 and SD Card Test");

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (SD.begin(SD_CS, SPI, 1000000)) {
    Serial.println("SUCCESS!");
    Serial.println("SD Card Communication Working!");
  } else {
    Serial.println("FAILED!");
    Serial.println("SD Card Communication Not Working!");
  }
}

void loop() {
}
