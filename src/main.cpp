#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <Adafruit_NeoPixel.h>

// ============================================================
// ESP32-S3 + PCF8575 8x7 matrix keypad test
// Includes startup I2C diagnostics and safe bus-error handling.
// ============================================================

// Expected PCF8575 address when A0, A1 and A2 are LOW.
// Change this if your address jumpers are configured differently.
#define PCF8575_ADDR 0x20

// ESP32-S3 N16R8 / DevKitC-1 I2C wiring.
// These are ESP32 GPIO numbers, NOT PCF8575 pin numbers.
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define I2C_FREQUENCY 100000

#define RGB_LED_PIN 48
#define RGB_LED_COUNT 1

Adafruit_NeoPixel onboardRgb(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

const uint8_t ROW_PINS[8] = {3, 11, 5, 6, 8, 2, 13, 14};
const uint8_t COL_PINS[7] = {10, 7, 0, 1, 9, 4, 12};

#define OK_ROW 5
#define OK_COL 4
#define SOS_ROW 1
#define SOS_COL 3

const char *keyMap[8][7] = {
    {"back", "2", "3", "v", "w", "", "radio"},
    {"7", "8", "9", "sos", "s", "", "y"},
    {"m", "n", "o", "p", "q", "r", "x"},
    {"g", "h", "i", "j", "k", "l", "0"},
    {"a", "b", "c", "d", "e", "f", ""},
    {"cl", "back", "up", "right", "ok", "", ""},
    {"4", "5", "6", "t", "u", "", "z"},
    {"shift", "space", "left", "down", "call_attend", "call_decline", ""}};

static bool pressLocked[8][7] = {false};
static bool shiftActive = false;
static bool capsLockActive = false;
static bool emergencyComboLatched = false;

static bool keypadSleeping = false;
static unsigned long lastKeyActivity = 0;
static unsigned long lastSleepCheck = 0;

static bool pcfReady = false;
static bool pcfFaultReported = false;

// Check whether a device acknowledges its address.
static bool i2cDevicePresent(uint8_t address)
{
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

// Scan the normal 7-bit I2C address range and report every response.
// The keypad expander must respond at PCF8575_ADDR.
static bool diagnoseI2C()
{
    uint8_t foundCount = 0;
    bool pcfFound = false;

    Serial.println();
    Serial.println("I2C BUS SCAN");
    Serial.println("------------");

    for (uint8_t address = 1; address < 127; address++)
    {
        if (i2cDevicePresent(address))
        {
            Serial.print("I2C device found at 0x");
            if (address < 0x10)
                Serial.print('0');
            Serial.println(address, HEX);

            foundCount++;
            if (address == PCF8575_ADDR)
                pcfFound = true;
        }
    }

    if (foundCount == 0)
        Serial.println("No I2C devices responded.");

    Serial.print("Devices found: ");
    Serial.println(foundCount);

    if (!pcfFound)
    {
        Serial.print("ERROR: PCF8575 not found at 0x");
        Serial.println(PCF8575_ADDR, HEX);
        Serial.println("Check VCC, GND, SDA, SCL, pull-ups and A0/A1/A2.");
        Serial.println("If the scanner shows another address, verify the address jumpers.");
        return false;
    }

    Serial.println("PCF8575 address acknowledged.");
    return true;
}

// Write a 16-bit state to the PCF8575.
// Returns false when the expander does not acknowledge the write.
static bool pcfWrite(uint16_t value)
{
    Wire.beginTransmission(PCF8575_ADDR);
    Wire.write((uint8_t)(value & 0xFF));
    Wire.write((uint8_t)(value >> 8));
    return Wire.endTransmission() == 0;
}

// Read both PCF8575 ports. Never use a partial/failed read as key data.
static bool pcfRead(uint16_t &value)
{
    uint8_t received = Wire.requestFrom((uint8_t)PCF8575_ADDR, (uint8_t)2);

    if (received != 2 || Wire.available() < 2)
    {
        while (Wire.available())
            Wire.read();
        return false;
    }

    uint16_t lowByte = (uint8_t)Wire.read();
    uint16_t highByte = (uint8_t)Wire.read();
    value = lowByte | (highByte << 8);
    return true;
}

static void reportPcfFault(const char *operation)
{
    pcfReady = false;

    if (!pcfFaultReported)
    {
        Serial.println();
        Serial.print("PCF8575 COMMUNICATION LOST during ");
        Serial.println(operation);
        Serial.println("Keypad scanning stopped to prevent false key events.");
        Serial.println("Check the I2C wiring and power, then reset the ESP32.");
        pcfFaultReported = true;
    }
}

static bool writePcfOrStop(uint16_t value)
{
    if (!pcfWrite(value))
    {
        reportPcfFault("I2C write");
        return false;
    }
    return true;
}

static bool readPcfOrStop(uint16_t &value)
{
    if (!pcfRead(value))
    {
        reportPcfFault("I2C read");
        return false;
    }
    return true;
}

static void processKey(const char *rawKey, void (*callback)(const char *))
{
    if (!rawKey || strlen(rawKey) == 0)
        return;

    Serial.print("Key Raw: ");
    Serial.println(rawKey);

    if (!strcmp(rawKey, "shf") || !strcmp(rawKey, "shift"))
    {
        shiftActive = true;
        return;
    }

    if (!strcmp(rawKey, "cl"))
    {
        capsLockActive = !capsLockActive;
        shiftActive = false;
        Serial.print("Caps Lock: ");
        Serial.println(capsLockActive ? "ON" : "OFF");
        return;
    }

    if (!strcmp(rawKey, "back"))
    {
        if (shiftActive)
        {
            shiftActive = false;
            callback("shift_back");
        }
        else
            callback("back");
        return;
    }

    if (!strcmp(rawKey, "ok"))
    {
        if (shiftActive)
        {
            shiftActive = false;
            callback("shift_ok");
        }
        else
            callback("ok");
        return;
    }

    if (!strcmp(rawKey, "up") || !strcmp(rawKey, "down") ||
        !strcmp(rawKey, "left") || !strcmp(rawKey, "right"))
    {
        callback(rawKey);
        shiftActive = false;
        return;
    }

    if (strlen(rawKey) == 1)
    {
        char c = rawKey[0];

        if (c >= 'a' && c <= 'z')
        {
            bool uppercase = shiftActive ^ capsLockActive;
            if (uppercase)
                c -= 32;
        }

        shiftActive = false;
        char out[2] = {c, '\0'};
        callback(out);
        return;
    }

    callback(rawKey);
    shiftActive = false;
}

static void keypadScan(void (*callback)(const char *))
{
    if (!pcfReady)
        return;

    unsigned long now = millis();

    if (!keypadSleeping && (now - lastKeyActivity > 5000))
    {
        keypadSleeping = true;
        Serial.println("Keypad sleeping");
    }

    if (keypadSleeping)
    {
        if (now - lastSleepCheck < 200)
            return;

        lastSleepCheck = now;

        for (int r = 0; r < 8; r++)
        {
            uint16_t out = 0xFFFF;
            out &= ~(1U << ROW_PINS[r]);

            for (int c = 0; c < 7; c++)
                out |= (1U << COL_PINS[c]);

            if (!writePcfOrStop(out))
                return;

            delayMicroseconds(200);

            uint16_t in;
            if (!readPcfOrStop(in))
                return;

            for (int c = 0; c < 7; c++)
            {
                if ((in & (1U << COL_PINS[c])) == 0)
                {
                    keypadSleeping = false;
                    lastKeyActivity = now;
                    Serial.println("Keypad woke");
                    return;
                }
            }
        }
        return;
    }

    for (int r = 0; r < 8; r++)
    {
        uint16_t out = 0xFFFF;
        out &= ~(1U << ROW_PINS[r]);

        for (int c = 0; c < 7; c++)
            out |= (1U << COL_PINS[c]);

        if (!writePcfOrStop(out))
            return;

        delayMicroseconds(200);

        uint16_t in;
        if (!readPcfOrStop(in))
            return;

        for (int c = 0; c < 7; c++)
        {
            bool pressed = ((in & (1U << COL_PINS[c])) == 0);

            if (pressed && !pressLocked[r][c])
            {
                pressLocked[r][c] = true;
                lastKeyActivity = now;

                bool sosPressed = pressLocked[SOS_ROW][SOS_COL] ||
                                  (r == SOS_ROW && c == SOS_COL);
                bool okPressed = pressLocked[OK_ROW][OK_COL] ||
                                 (r == OK_ROW && c == OK_COL);

                if (sosPressed && okPressed)
                {
                    if (!emergencyComboLatched)
                    {
                        emergencyComboLatched = true;
                        Serial.println("Key Combo: emergency_alert");
                        callback("emergency_alert");
                    }
                    continue;
                }

                if (keyMap[r][c] && strlen(keyMap[r][c]) > 0)
                {
                    Serial.print("[KEYPAD] Physical key pressed: ");
                    Serial.println(keyMap[r][c]);
                    processKey(keyMap[r][c], callback);
                }
            }

            if (!pressed)
                pressLocked[r][c] = false;
        }

        delayMicroseconds(500);
    }

    if (!pressLocked[SOS_ROW][SOS_COL] || !pressLocked[OK_ROW][OK_COL])
        emergencyComboLatched = false;
}

static void onKeyEvent(const char *key)
{
    Serial.print("KEY EVENT: ");
    Serial.println(key);
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    onboardRgb.begin();
    onboardRgb.clear();
    onboardRgb.show();

    Serial.println();
    Serial.println("ESP32-S3 PCF8575 KEYPAD TEST");
    Serial.println("===========================");
    Serial.print("I2C SDA: GPIO");
    Serial.println(I2C_SDA_PIN);
    Serial.print("I2C SCL: GPIO");
    Serial.println(I2C_SCL_PIN);
    Serial.print("Expected PCF8575 address: 0x");
    Serial.println(PCF8575_ADDR, HEX);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
    Wire.setTimeOut(50);

    if (!diagnoseI2C())
    {
        Serial.println();
        Serial.println("FIX THE I2C CONNECTION, THEN RESET THE BOARD.");
        return;
    }

    // Set all expander pins HIGH (released/input state).
    if (!writePcfOrStop(0xFFFF))
        return;

    uint16_t initialState;
    if (!readPcfOrStop(initialState))
        return;

    pcfReady = true;
    lastKeyActivity = millis();

    Serial.println();
    Serial.println("PCF8575 read/write test: PASSED");
    Serial.println("Keypad is ready. Press any key.");
}

void loop()
{
    if (!pcfReady)
    {
        delay(500);
        return;
    }

    keypadScan(onKeyEvent);
}
