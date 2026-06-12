#ifndef WLAN_H
#define WLAN_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <globaldefines.h>
//Name für den Access Point im WiFi-Provisioning-Modus
#define AP_SSID "SoilSense-Setup"
#define PREFS_NAMESPACE "storage"
#define PREFS_KEY_SSID "field1"
#define PREFS_KEY_PASSWORD "field2"

extern PubSubClient mqttClient;
typedef void (*MqttCallbackFn)(char*, uint8_t*, unsigned int);
void mqtt_setCallback(MqttCallbackFn cb);
bool wifiReady();
extern WebServer server;
void publishdata(String capability, String Value);
void publishRegistration();
void mqttloop();
bool mqttcheckconnection();
void reconnectMQTT();
void mqtt_subscribeToTopics(void);
void mqtt_init();
bool wlan_connectToWiFi(const String& ssid, const String& password);
void wlan_startAccessPoint();
void wlan_stopAccessPoint();
void wlan_handleRoot();
void wlan_handleSave();
void wlan_loadCredentials(String& ssid, String& password);
void wlan_saveCredentials(const String& ssid, const String& password);
void wlan_clearCredentials();
void wlan_init();
void wlan_clearPreferences();
void wlan_saveMacAddress(const String& mac);
String wlan_loadMacAddress();
String wlan_getPairingCode();
String buildRegistrationPayload(const String& deviceId, const String& thingModelFragment);
// Build capabilities doc from thing model (call once at startup)
bool buildCapabilitiesDocFromThingModel(const String& thingModelFragment);

// Simple doc-free getters/setters that operate on the internal gCapabilitiesDoc
bool wlan_setCapabilityValue(const char* capability, const char* value);
String wlan_getCapabilityValue(const char* capability);
#endif
