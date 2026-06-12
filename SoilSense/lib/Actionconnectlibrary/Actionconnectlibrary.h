#ifndef ACTION_CONNECT_LIBRARY_H
#define ACTION_CONNECT_LIBRARY_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "Wlan.h"
#include "BLEManager.h"

// Device and broker configuration
struct ConnectHubDeviceInfo {
	const char* deviceId;
	const char* label;
	const char* category;
	const char* transportName;
};

struct ConnectHubBrokerConfig {
	const char* host;
	int port;
	const char* user;
	const char* password;
	uint16_t mqttBufferSize;
};

struct ConnectHubTopics {
	char command[64];
	char status[64];
	char telemetry[64];
};

typedef void (*ConnectHubReceiveCallback)(const char* source, const char* topic, const String& payload);

struct ConnectHubTransportConfig {
	const ConnectHubDeviceInfo* device;
	const ConnectHubBrokerConfig* broker;
};

// Protocol helpers

String connectHubBuildStatusPayload(const char* status);
String connectHubExtractJsonValue(const String& json, const char* key);

// Transport helpers


void connectHubProcessTransport();
void connectHubSetReceiveCallback(ConnectHubReceiveCallback cb);

void connectHubSend(String capability,String value);

// Publish a raw telemetry payload string to devices/{mac}/telemetry
void publish_telemetry(const String& payload);

// Publish wrapper (calls Wlan::publishdata)
void MQTTsenddata(const String& capability, const String& value);

// Initialize transports + MQTT wiring
void connectHubInit();
// convenience wrappers for consumers
String connectHubGetCapability(const char* capability);
bool connectHubSetCapability(const char* capability, const char* value);
#endif // ACTION_CONNECT_LIBRARY_H
