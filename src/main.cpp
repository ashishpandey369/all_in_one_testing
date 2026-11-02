#include <Arduino.h>
#include <Wire.h>
#include <string.h>

// ============================================================
// Pocket_Node keypad test port for ESP32-S3
// PCF8575 8x7 matrix keypad
// ============================================================

#define PCF8575_ADDR 0x20

// ESP32-S3 DevKitC-1 I2C pins.
// Change these two values if your SDA/SCL wires use different GPIOs.
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// PCF8575 matrix pin assignments
const uint8_t ROW_PINS[8] = {3, 11, 5, 6, 8, 2, 13, 14};
const uint8_t COL_PINS[7] = {10, 7, 0, 1, 9, 4, 12};

// Special key positions
#define OK_ROW 5
#define OK_COL 4
#define SOS_ROW 1
#define SOS_COL 3

// Key map copied from Pocket_Node
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

// Keypad idle/sleep control
static bool keypadSleeping = false;
static unsigned long lastKeyActivity = 0;
static unsigned long lastSleepCheck = 0;

// Write 16-bit output state to PCF8575
static void pcfWrite(uint16_t value)
{
    Wire.beginTransmission(PCF8575_ADDR);
    Wire.write(value & 0xFF);
    Wire.write(value >> 8);
    Wire.endTransmission();
}

// Read 16-bit input state from PCF8575
static uint16_t pcfRead()
{
    Wire.requestFrom((uint8_t)PCF8575_ADDR, (uint8_t)2);
    if (Wire.available() < 2)
        return 0xFFFF;

    uint16_t lowByte = Wire.read();
    uint16_t highByte = Wire.read();
    return lowByte | (highByte << 8);
}

// Process modifiers and special keys before sending key text
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
        {
            callback("back");
        }
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
        {
            callback("ok");
        }
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

// Scan the keypad matrix and report key events
static void keypadScan(void (*callback)(const char *))
{
    unsigned long now = millis();

    // Enter sleep after 5 seconds without key activity
    if (!keypadSleeping && (now - lastKeyActivity > 5000))
    {
        keypadSleeping = true;
        Serial.println("Keypad sleeping");
    }

    // Sleep scan: poll every 200 ms and wake on any key
    if (keypadSleeping)
    {
        if (now - lastSleepCheck < 200)
            return;

        lastSleepCheck = now;

        for (int r = 0; r < 8; r++)
        {
            uint16_t out = 0xFFFF;
            out &= ~(1 << ROW_PINS[r]);

            for (int c = 0; c < 7; c++)
                out |= (1 << COL_PINS[c]);

            pcfWrite(out);
            delayMicroseconds(200);
            uint16_t in = pcfRead();

            for (int c = 0; c < 7; c++)
            {
                if ((in & (1 << COL_PINS[c])) == 0)
                {
                    keypadSleeping = false;
                    lastKeyActivity = now;
                    Serial.println("Keypad woke");
                    return; // Do not fire the wake-up key
                }
            }
        }
        return;
    }

    // Normal matrix scan
    for (int r = 0; r < 8; r++)
    {
        uint16_t out = 0xFFFF;
        out &= ~(1 << ROW_PINS[r]);

        for (int c = 0; c < 7; c++)
            out |= (1 << COL_PINS[c]);

        pcfWrite(out);
        delayMicroseconds(200);

        uint16_t in = pcfRead();

        for (int c = 0; c < 7; c++)
        {
            bool pressed = ((in & (1 << COL_PINS[c])) == 0);

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

                Serial.print("Key Pressed: ");
                Serial.println(keyMap[r][c]);

                if (keyMap[r][c])
                    processKey(keyMap[r][c], callback);
            }

            if (!pressed)
                pressLocked[r][c] = false;
        }

        // Yield briefly between rows
        delayMicroseconds(500);
    }

    if (!pressLocked[SOS_ROW][SOS_COL] || !pressLocked[OK_ROW][OK_COL])
        emergencyComboLatched = false;
}

// Print each processed key to Serial Monitor
static void onKeyEvent(const char *key)
{
    Serial.print("KEY EVENT: ");
    Serial.println(key);
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    // Start I2C once; keypadScan uses this same bus.
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);

    pcfWrite(0xFFFF);
    lastKeyActivity = millis();

    Serial.println();
    Serial.println("PCF8575 Keypad Test Started");
    Serial.print("I2C SDA: GPIO");
    Serial.println(I2C_SDA_PIN);
    Serial.print("I2C SCL: GPIO");
    Serial.println(I2C_SCL_PIN);
    Serial.println("PCF8575 address: 0x20");
    Serial.println("Press any key...");
}

void loop()
{
    keypadScan(onKeyEvent);
}
