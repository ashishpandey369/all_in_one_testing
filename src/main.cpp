#include <Arduino.h>
#include "components/display/display.h"

void setup()
{
    Serial.begin(115200);
    delay(200);

    displayInit();
    displayShowHi();

    Serial.println("ILI9341 display test: HI is shown.");
}

void loop()
{
    // Display remains active. Other components will be integrated here later.
    delay(1000);
}
