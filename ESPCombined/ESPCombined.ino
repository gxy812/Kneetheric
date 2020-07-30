// Original Source https://gist.github.com/manuelbl/66f059effc8a7be148adb1f104666467
/* 
 * Sample program for ESP32 acting as a Bluetooth keyboard
 * 
 * Copyright (c) 2019 Manuel Bl
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 */
//Act as bluetooth keyboard, use ultrasonic sensor and buzzer

//Pins
#define TRIG 14
#define ECHO 27
#define BUZZ 15
//Bluetooth
#define US_KEYBOARD 1
// Change the below values if desired
//#define BUTTON_PIN 0 //boot button
#define GOOD "w"
#define ERR "s"
#define DEVICE_NAME "Kneetheric Device"
//distance of sensor from inner knee
#define SENSOR_DIS 10
//angle of sensor off from thigh
#define SENSOR_ANG radians(5)
#define MAX_STORE 2

#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"

//forward declarations
void bluetoothTask(void *);
void typeText(const char *text);
void onError();

//globals
bool isBleConnected = false;
long loop_start = 0;
long last_angle = 0;
long last_exec = 0;
float current_v = 0;
float current_angle = 0;
bool check_av = false;
bool extending = false;

class AngleStore
{
    float angle[MAX_STORE] = {};
    int i = 0;

public:
    float getAngle()
    {
        float dist;
        float opp;
        // Clears TRIG
        digitalWrite(TRIG, LOW);
        delayMicroseconds(2);

        // Sets the TRIG on HIGH state for 10 micro seconds
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);

        // Reads ECHO, returns the sound wave travel time in microseconds, then multiplies to get distance in cm
        dist = pulseIn(ECHO, HIGH) * 0.034 / 2;
        /*
        ||     _________
        ||    / ________ -> SENSOR_DIS
        ||   / /angle) ^ -> SENSOR_ANG
        ||  / /
        || / /->dist
        */
        // Get opposite side
        opp = sqrt(SENSOR_DIS * SENSOR_DIS + dist * dist + 2 * SENSOR_DIS * dist * cos(SENSOR_ANG));
        // Angle
        // use sin rule
        dist = asin(dist * sin(SENSOR_ANG) / opp);
        dist = degrees(dist);
        // store angle for velocity calc
        angle[i] = dist;
        ++i;
        if (i >= MAX_STORE)
        {
            i = 0;
        }
        return dist;
    }
    float getAngularVelocity()
    {
        float v = angle[i];
        if ((i - 1) < 0)
        {
            v = (v - angle[MAX_STORE - 1]) / 0.1;
        }
        else
        {
            //get rad per sec
            v = (v - angle[i - 1]) / 0.1;
        }
        //absolute velocity
        v = abs(v);
        //convert to degree
        return degrees(v);
    }
};
AngleStore ang;

void setup()
{
    Serial.begin(115200);

    // configure pins
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT_PULLDOWN);
    //channel, freq, bit-resolution
    ledcSetup(0, 2000, 8);
    //pin, channel
    ledcAttachPin(BUZZ, 0);

    // start Bluetooth task
    xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);
}

void loop()
{
    loop_start = millis();
    // every 100 ms read angle
    if (loop_start - last_angle > 100)
    {
        current_angle = ang.getAngle();
        last_angle = millis();
    }
    if (check_av)
    { // read angular velocity after first loop
        current_v = ang.getAngularVelocity();
    }
    else
        check_av = true;
    // check for correct angle change and velocity every 50 ms
    if (isBleConnected & (loop_start - last_exec > 50))
    {
        ledcWriteTone(0, 0);
        if (current_v < 5)
        {
            //leg is extended and supposed to be extended
            if (current_angle >= 170 && current_angle <= 180 && extending)
            {
                Serial.println(GOOD);
                typeText(GOOD);
                extending = false;
            }
            //leg is bent and supposed to be bent
            else if (current_angle >= 85 && current_angle <= 90 && !extending)
            {
                Serial.println(GOOD);
                typeText(GOOD);
                extending = true;
            }
        }
        else
        {
            onError();
        }
        last_exec = millis();
    }
}

//send warning via buzzer and take a step back
void onError()
{
    Serial.println(ERR);
    typeText(ERR);
    //channel, freq
    ledcWriteTone(0, 4000);
}

// Message (report) sent when a key is pressed or released
struct InputReport
{
    uint8_t modifiers;      // bitmask: CTRL = 1, SHIFT = 2, ALT = 4
    uint8_t reserved;       // must be 0
    uint8_t pressedKeys[6]; // up to six concurrenlty pressed keys
};

// Message (report) received when an LED's state changed
struct OutputReport
{
    uint8_t leds; // bitmask: num lock = 1, caps lock = 2, scroll lock = 4, compose = 8, kana = 16
};

// The report map describes the HID device (a keyboard in this case) and
// the messages (reports in HID terms) sent and received.
static const uint8_t REPORT_MAP[] = {
    USAGE_PAGE(1), 0x01,      // Generic Desktop Controls
    USAGE(1), 0x06,           // Keyboard
    COLLECTION(1), 0x01,      // Application
    REPORT_ID(1), 0x01,       //   Report ID (1)
    USAGE_PAGE(1), 0x07,      //   Keyboard/Keypad
    USAGE_MINIMUM(1), 0xE0,   //   Keyboard Left Control
    USAGE_MAXIMUM(1), 0xE7,   //   Keyboard Right Control
    LOGICAL_MINIMUM(1), 0x00, //   Each bit is either 0 or 1
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_COUNT(1), 0x08, //   8 bits for the modifier keys
    REPORT_SIZE(1), 0x01,
    HIDINPUT(1), 0x02,     //   Data, Var, Abs
    REPORT_COUNT(1), 0x01, //   1 byte (unused)
    REPORT_SIZE(1), 0x08,
    HIDINPUT(1), 0x01,     //   Const, Array, Abs
    REPORT_COUNT(1), 0x06, //   6 bytes (for up to 6 concurrently pressed keys)
    REPORT_SIZE(1), 0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65, //   101 keys
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(1), 0x65,
    HIDINPUT(1), 0x00,     //   Data, Array, Abs
    REPORT_COUNT(1), 0x05, //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
    REPORT_SIZE(1), 0x01,
    USAGE_PAGE(1), 0x08,    //   LEDs
    USAGE_MINIMUM(1), 0x01, //   Num Lock
    USAGE_MAXIMUM(1), 0x05, //   Kana
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    HIDOUTPUT(1), 0x02,    //   Data, Var, Abs
    REPORT_COUNT(1), 0x01, //   3 bits (Padding)
    REPORT_SIZE(1), 0x03,
    HIDOUTPUT(1), 0x01, //   Const, Array, Abs
    END_COLLECTION(0)   // End application collection
};

BLEHIDDevice *hid;
BLECharacteristic *input;
BLECharacteristic *output;

const InputReport NO_KEY_PRESSED = {};

/*
 * Callbacks related to BLE connection
 */
class BleKeyboardCallbacks : public BLEServerCallbacks
{

    void onConnect(BLEServer *server)
    {
        isBleConnected = true;

        // Allow notifications for characteristics
        BLE2902 *cccDesc = (BLE2902 *)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        cccDesc->setNotifications(true);

        Serial.println("Client has connected");
    }

    void onDisconnect(BLEServer *server)
    {
        isBleConnected = false;

        // Disallow notifications for characteristics
        BLE2902 *cccDesc = (BLE2902 *)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        cccDesc->setNotifications(false);

        Serial.println("Client has disconnected");
    }
};

/*
 * Called when the client (computer, smart phone) wants to turn on or off
 * the LEDs in the keyboard.
 * 
 * bit 0 - NUM LOCK
 * bit 1 - CAPS LOCK
 * bit 2 - SCROLL LOCK
 */
class OutputCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {
        OutputReport *report = (OutputReport *)characteristic->getData();
        Serial.print("LED state: ");
        Serial.print((int)report->leds);
        Serial.println();
    }
};

void bluetoothTask(void *)
{

    // initialize the device
    BLEDevice::init(DEVICE_NAME);
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new BleKeyboardCallbacks());

    // create an HID device
    hid = new BLEHIDDevice(server);
    input = hid->inputReport(1);   // report ID
    output = hid->outputReport(1); // report ID
    output->setCallbacks(new OutputCallbacks());

    // set manufacturer name
    hid->manufacturer()->setValue("Maker Community");
    // set USB vendor and product ID
    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    // information about HID device: device is not localized, device can be connected
    hid->hidInfo(0x00, 0x02);

    // Security: device requires bonding
    BLESecurity *security = new BLESecurity();
    security->setAuthenticationMode(ESP_LE_AUTH_BOND);

    // set report map
    hid->reportMap((uint8_t *)REPORT_MAP, sizeof(REPORT_MAP));
    hid->startServices();

    // set battery level to 100%
    hid->setBatteryLevel(100);

    // advertise the services
    BLEAdvertising *advertising = server->getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->addServiceUUID(hid->deviceInfo()->getUUID());
    advertising->addServiceUUID(hid->batteryService()->getUUID());
    advertising->start();

    Serial.println("BLE ready");
    delay(portMAX_DELAY);
};

void typeText(const char *text)
{
    int len = strlen(text);
    for (int i = 0; i < len; i++)
    {

        // translate character to key combination
        uint8_t val = (uint8_t)text[i];
        if (val > KEYMAP_SIZE)
            continue; // character not available on keyboard - skip
        KEYMAP map = keymap[val];

        // create input report
        InputReport report = {
            .modifiers = map.modifier,
            .reserved = 0,
            .pressedKeys = {
                map.usage,
                0, 0, 0, 0, 0}};

        // send the input report
        input->setValue((uint8_t *)&report, sizeof(report));
        input->notify();

        delay(5);

        // release all keys between two characters; otherwise two identical
        // consecutive characters are treated as just one key press
        input->setValue((uint8_t *)&NO_KEY_PRESSED, sizeof(NO_KEY_PRESSED));
        input->notify();

        delay(5);
    }
}
