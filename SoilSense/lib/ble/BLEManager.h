#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

typedef void (*PinCommandCallback)(int pin, int value);
typedef void (*BLELineCallback)(const String& line);

// BLE UUIDs are persisted per-device. Use the getters below to access the
// service/characteristic UUIDs. They are generated once and stored in
// Preferences so they survive reboots and re-flashes.

String getBleServiceUuid();
String getBleTxUuid();
String getBleRxUuid();
String getDeviceId();

void setupBLE(const char* deviceName = "ESP32-BLE");
bool isBLEConnected();
void sendBLEValue(float value, int decimals = 2);
void sendBLEJson(const String& payload);
void setBLEPinCommandCallback(PinCommandCallback cb);
void setBLELineCallback(BLELineCallback cb);
void processBLE();

#endif
