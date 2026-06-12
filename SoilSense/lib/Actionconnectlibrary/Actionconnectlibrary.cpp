#include "Actionconnectlibrary.h"

#include <BLEManager.h>
#include <ArduinoJson.h>
#include <stdio.h>

namespace {
ConnectHubReceiveCallback g_receiveCallback = nullptr;
const unsigned long WIFI_PROVISION_AP_FALLBACK_MS = 300000;
unsigned long g_bleProvisioningStartedAt = 0;
bool g_accessPointStarted = false;

void dispatchIncoming(const char* source, const char* topic, const String& payload) {
	if (!g_receiveCallback) {
		return;
	}

	g_receiveCallback(source, topic, payload);
}

void sendBleProvisioningResult(const char* status, const char* error = nullptr) {
	String payload = String("{\"provision\":\"") + status + "\"";
	if (error) {
		payload += String(",\"error\":\"") + error + "\"";
	}
	if (wifiReady()) {
		payload += String(",\"ip\":\"") + WiFi.localIP().toString() + "\"";
	}
	payload += "}";
	sendBLEJson(payload);
	Serial.print("[BLE] Provisioning result: ");
	Serial.println(payload);
}

bool parseWifiProvisioningJson(const String& payload, String& ssid, String& password) {
	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, payload);
	if (err) {
		return false;
	}

	const char* type = doc["type"] | "";
	bool looksLikeWifiProvisioning =
			String(type) == "wifi" ||
			String(type) == "wifi_provisioning" ||
			(doc["ssid"].is<const char*>() && doc["password"].is<const char*>());

	if (!looksLikeWifiProvisioning) {
		return false;
	}

	ssid = String(doc["ssid"] | "");
	password = String(doc["password"] | "");
	ssid.trim();
	return true;
}

bool parseWifiProvisioningText(const String& payload, String& ssid, String& password) {
	if (!payload.startsWith("WIFI:")) {
		return false;
	}

	int separator = payload.indexOf(':', 5);
	if (separator < 0) {
		return false;
	}

	ssid = payload.substring(5, separator);
	password = payload.substring(separator + 1);
	ssid.trim();
	return true;
}

bool handleWifiClearMessage(const String& payload) {
	if (payload == "WIFI:CLEAR") {
		wlan_clearCredentials();
		sendBleProvisioningResult("cleared");
		return true;
	}

	if (!payload.startsWith("{")) {
		return false;
	}

	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, payload);
	if (err) {
		return false;
	}

	const char* type = doc["type"] | "";
	if (String(type) != "wifi_clear") {
		return false;
	}

	wlan_clearCredentials();
	sendBleProvisioningResult("cleared");
	return true;
}

bool handleWifiProvisioningMessage(const String& payload) {
	if (handleWifiClearMessage(payload)) {
		return true;
	}

	String ssid;
	String password;

	bool isProvisioningMessage = payload.startsWith("{")
			? parseWifiProvisioningJson(payload, ssid, password)
			: parseWifiProvisioningText(payload, ssid, password);

	if (!isProvisioningMessage) {
		return false;
	}

	if (ssid.length() == 0) {
		sendBleProvisioningResult("err", "missing_ssid");
		return true;
	}

	Serial.print("[BLE] Provisioning WiFi SSID: ");
	Serial.println(ssid);

	wlan_saveCredentials(ssid, password);
	if (wlan_connectToWiFi(ssid, password)) {
		wlan_saveMacAddress(WiFi.macAddress());
		if (g_accessPointStarted) {
			wlan_stopAccessPoint();
			g_accessPointStarted = false;
		}
		sendBleProvisioningResult("ok");

		if (!mqttcheckconnection()) {
			reconnectMQTT();
		}
		if (mqttcheckconnection()) {
			mqtt_subscribeToTopics();
			publishRegistration();
		}
	} else {
		sendBleProvisioningResult("err", "wifi_connect_failed");
	}

	return true;
}

void onBleMessage(const String& payload) {
	if (handleWifiProvisioningMessage(payload)) {
		return;
	}

	dispatchIncoming("ble", "", payload);
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
	String message;
	message.reserve(length);

	Serial.print("Message received on topic: ");
	Serial.println(topic);
	Serial.print("Payload: ");

	for (unsigned int i = 0; i < length; i++) {
		char c = static_cast<char>(payload[i]);
		Serial.print(c);
		message += c;
	}
	Serial.println();

	String mac = wlan_loadMacAddress();
	String commandTopic = "devices/" + mac + "/command";
	String controlTopic = "devices/" + mac + "/control";

	if (String(topic) == commandTopic || String(topic) == controlTopic) {
		dispatchIncoming("mqtt", topic, message);
	}
}

void broadcastLocalPayload(const String& payload) {
	if (isBLEConnected()) {
		sendBLEJson(payload);
		Serial.print("[BLE] Sent: ");
		Serial.println(payload);
	}
}
}

void connectHubProcessTransport() {
	mqttloop();
	processBLE();

	if (!wifiReady() &&
			!g_accessPointStarted &&
			!isBLEConnected() &&
			millis() - g_bleProvisioningStartedAt >= WIFI_PROVISION_AP_FALLBACK_MS) {
		wlan_startAccessPoint();
		g_accessPointStarted = true;
	}
	
}

void connectHubSetReceiveCallback(ConnectHubReceiveCallback cb) {
	g_receiveCallback = cb;
}

//sendet erst per mqtt, dann per ble
void connectHubSend(String capability,String value)
{
	//first try mqtt
	MQTTsenddata(capability, value);

	//then try ble
	broadcastLocalPayload(capability+":"+value);
}
// Publish a raw telemetry payload string to devices/{mac}/telemetry
//User can choose their own format, just make sure It is a viable one
void publish_telemetry(const String& payload) {
	String mac = wlan_loadMacAddress();
	String topic = "devices/" + mac + "/telemetry";
	if (mqttClient.publish(topic.c_str(), payload.c_str())) {
		Serial.print("Published telemetry to ");
		Serial.print(topic);
		Serial.print(": ");
		Serial.println(payload);
	} else {
		Serial.print("Failed to publish telemetry to ");
		Serial.println(topic);
		Serial.print("MQTT state: ");
		Serial.println(mqttClient.state());
	}
}
// Wrapper that forwards to wlan::publishdata()
//schreibt capability in interne datenbank und schickt den wert dann per mqtt
void MQTTsenddata(const String& capability, const String& value) {
	connectHubSetCapability(capability.c_str(), value.c_str());
    publishdata(capability, value);
}

// Initialize transports and prepare WiFi, MQTT, and BLE.
void connectHubInit() {
  WiFi.mode(WIFI_AP_STA);
	// setupBLE("Gerätename hier")
	setupBLE("SoilSense");
	setBLELineCallback(onBleMessage);
	g_bleProvisioningStartedAt = millis();

  String ssid, password;
  wlan_loadCredentials(ssid, password);
  bool hasStoredCredentials = ssid.length() > 0;
  bool connected = false;
	if (hasStoredCredentials) {
		connected = wlan_connectToWiFi(ssid, password);
		// If we had stored credentials but couldn't connect, fall back to AP
		if (!connected && !isBLEConnected()) {
			wlan_startAccessPoint();
			g_accessPointStarted = true;
		}
	} else {
		// No stored credentials -> start AP immediately for provisioning
		if (!isBLEConnected()) {
			wlan_startAccessPoint();
			g_accessPointStarted = true;
		}
	}

  wlan_init();

  String mac = WiFi.macAddress();
  wlan_saveMacAddress(mac);

  mqtt_init();
  mqtt_setCallback(onMqttMessage);
  if (wifiReady()) {
    reconnectMQTT();
  }

  // Subscribe after first connect
  if (mqttcheckconnection()) {
    mqtt_subscribeToTopics();
  }
}

String connectHubBuildStatusPayload(const char* status) {
	return String("{\"status\":\"") + status + "\"}";
}

String connectHubExtractJsonValue(const String& json, const char* key) {
	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, json);
	if (err) {
		return "";
	}

	JsonVariant value = doc[key];
	if (!value) {
		return "";
	}

	if (value.is<const char*>()) {
		return String(value.as<const char*>());
	}

	String result;
	serializeJson(value, result);
	return result;
}

//liest capability aus interner Datenbank
String connectHubGetCapability(const char* capability) {
  return wlan_getCapabilityValue(capability);
}
//schreibt in interne datenbank
bool connectHubSetCapability(const char* capability, const char* value) {
  return wlan_setCapabilityValue(capability, value);
}
