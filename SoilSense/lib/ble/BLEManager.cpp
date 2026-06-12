#include "BLEManager.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Preferences.h>
#include <esp_system.h>

static BLEServer* pServer = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static bool deviceConnected = false;
static PinCommandCallback g_pinCb = nullptr;
static BLELineCallback g_lineCb = nullptr;
static bool gIdentityLogged = false;

class BleCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    deviceConnected = true;
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer* server) override {
    deviceConnected = false;
    Serial.println("BLE client disconnected, restarting advertising");
    server->startAdvertising();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    std::string raw = characteristic->getValue();
    if (raw.empty()) {
      return;
    }

    String line = String(raw.c_str());
    line.trim();

    Serial.print("BLE received: ");
    Serial.println(line);

    if (g_lineCb) {
      g_lineCb(line);
    }

    if (line.startsWith("PIN:")) {
      int firstColon = line.indexOf(':');
      int secondColon = line.indexOf(':', firstColon + 1);
      if (firstColon > 0 && secondColon > firstColon && g_pinCb) {
        int pin = line.substring(firstColon + 1, secondColon).toInt();
        int value = line.substring(secondColon + 1).toInt();
        g_pinCb(pin, value);
      }
      return;
    }

    if (line.startsWith("LED:") && g_pinCb) {
      String arg = line.substring(4);
      arg.trim();
      arg.toUpperCase();

      int value = -1;
      if (arg == "ON" || arg == "HIGH") {
        value = 255;
      } else if (arg == "OFF" || arg == "LOW") {
        value = 0;
      } else {
        value = constrain(arg.toInt(), 0, 255);
      }

      if (value >= 0) {
        g_pinCb(2, value);
      }
    }
  }
};

void setupBLE(const char* deviceName) {
  BLEDevice::init(deviceName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleCallbacks());

  String serviceUuid = getBleServiceUuid();
  String txUuid = getBleTxUuid();
  String rxUuid = getBleRxUuid();
  String deviceId = getDeviceId();

  if (!gIdentityLogged) {
    Serial.println("BLE identity loaded:");
    Serial.print("  device_id: ");
    Serial.println(deviceId);
    Serial.print("  service_uuid: ");
    Serial.println(serviceUuid);
    Serial.print("  tx_uuid: ");
    Serial.println(txUuid);
    Serial.print("  rx_uuid: ");
    Serial.println(rxUuid);
    gIdentityLogged = true;
  }

  BLEService* service = pServer->createService(serviceUuid.c_str());
  pTxCharacteristic = service->createCharacteristic(
    txUuid.c_str(),
    BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic* rxCharacteristic = service->createCharacteristic(
    rxUuid.c_str(),
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rxCharacteristic->setCallbacks(new RxCallbacks());

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(service->getUUID());
  advertising->setScanResponse(true);
  advertising->start();

  Serial.print("BLE started: ");
  Serial.println(deviceName);
}

// --- persistent UUID helpers ---
static String makeUuidV4() {
  uint8_t b[16];
  for (int i = 0; i < 16; ++i) b[i] = (uint8_t)esp_random();
  b[6] = (b[6] & 0x0F) | 0x40; // version 4
  b[8] = (b[8] & 0x3F) | 0x80; // variant
  char s[37];
  snprintf(s, sizeof(s),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
  return String(s);
}

static String ensureBleUuid(const char* key) {
  Preferences prefs;
  prefs.begin("ble", false);
  String v = prefs.getString(key, "");
  if (v.length() == 0) {
    v = makeUuidV4();
    prefs.putString(key, v);
  }
  prefs.end();
  return v;
}

String getBleServiceUuid() { return ensureBleUuid("service_uuid"); }
String getBleTxUuid() { return ensureBleUuid("tx_uuid"); }
String getBleRxUuid() { return ensureBleUuid("rx_uuid"); }

static String makeDeviceId() {
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char id[7];
  for (int i = 0; i < 6; ++i) {
    id[i] = alphabet[esp_random() % (sizeof(alphabet) - 1)];
  }
  id[6] = '\0';
  return String(id);
}

static String ensureDeviceId() {
  Preferences prefs;
  prefs.begin("device", false);
  String v = prefs.getString("device_id", "");
  if (v.length() == 0) {
    v = makeDeviceId();
    prefs.putString("device_id", v);
  }
  prefs.end();
  return v;
}

String getDeviceId() { return ensureDeviceId(); }

bool isBLEConnected() {
  return deviceConnected;
}

void sendBLEValue(float value, int decimals) {
  if (!pTxCharacteristic) {
    return;
  }

  char buffer[32];
  int length = snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
  pTxCharacteristic->setValue((uint8_t*)buffer, length);
  pTxCharacteristic->notify();
}

void sendBLEJson(const String& payload) {
  if (!pTxCharacteristic) {
    return;
  }

  pTxCharacteristic->setValue(payload.c_str());
  pTxCharacteristic->notify();
}

void setBLEPinCommandCallback(PinCommandCallback cb) {
  g_pinCb = cb;
}

void setBLELineCallback(BLELineCallback cb) {
  g_lineCb = cb;
}

void processBLE() {
}
