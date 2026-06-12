#include "wlan.h"
// Thing model and device id constants
#include "../Actionconnectlibrary/ThingModel.h"
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
static Preferences prefs;
static unsigned long lastWifiNotConnectedLog = 0;
static unsigned long lastMqttReconnectAttempt = 0;
static const unsigned long CONNECTION_LOG_INTERVAL_MS = 10000;

// HTML Page content
String buildWifiConfigPage() {
  String html = F(
      "<!DOCTYPE html>"
      "<html>"
      "<body>"
      "<h2>ESP32 WiFi Setup</h2>"
      "<p>AccessCode: <strong>");
  html += wlan_getPairingCode();
  html += F(
      "</strong></p>"
      "<form action='/save' method='GET'>"
      "SSID: <input type='text' name='field1'><br><br>"
      "Password: <input type='password' name='field2'><br><br>"
      "<input type='submit' value='Save'>"
      "</form>"
      "</body>"
      "</html>");
  return html;
}

/**
 * @brief Load WiFi credentials from persistent storage
 * @param ssid Reference to store loaded SSID
 * @param password Reference to store loaded password
 */
void wlan_loadCredentials(String& ssid, String& password) {
  prefs.begin(PREFS_NAMESPACE, true);
  ssid = prefs.getString(PREFS_KEY_SSID, "");
  password = prefs.getString(PREFS_KEY_PASSWORD, "");
  prefs.end();

  Serial.print("Loaded WiFi SSID: ");
  Serial.println(ssid.length() > 0 ? ssid : "<none>");
}

/**
 * @brief Save WiFi credentials to persistent storage
 * @param ssid SSID to save
 * @param password Password to save
 */
void wlan_saveCredentials(const String& ssid, const String& password) {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString(PREFS_KEY_SSID, ssid);
  prefs.putString(PREFS_KEY_PASSWORD, password);
  prefs.end();
}

void wlan_clearCredentials() {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.remove(PREFS_KEY_SSID);
  prefs.remove(PREFS_KEY_PASSWORD);
  prefs.end();
  Serial.println("WiFi credentials cleared!");
}

/**
 * @brief Connect to WiFi network
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return true if connection successful, false otherwise
 */
bool wlan_connectToWiFi(const String& ssid, const String& password) {
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 16) {
    delay(500);
    attempts++;
    if (attempts % 8 == 0) {
      Serial.println("Still connecting to WiFi...");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("\nFailed to connect.");
  return false;
}

/**
 * @brief Start ESP32 as Access Point
 */
void wlan_startAccessPoint(void) {
  Serial.print("Setting Access Point…");
  WiFi.softAP(AP_SSID);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("Pairing Code: ");
  Serial.println(wlan_getPairingCode());
  
}

void wlan_stopAccessPoint(void) {
  WiFi.softAPdisconnect(true);
  Serial.println("Access Point stopped");
}

/**
 * @brief Handle HTTP GET request to root
 */
void wlan_handleRoot(void) {
  server.send(200, "text/html", buildWifiConfigPage());
}

/**
 * @brief Handle HTTP GET request to /save endpoint
 */
void wlan_handleSave(void) {
  String field1 = server.arg("field1");
  String field2 = server.arg("field2");

  wlan_saveCredentials(field1, field2);

  // Reload credentials from storage
  String ssid, password;
  wlan_loadCredentials(ssid, password);

  // Try to connect immediately and inform the user of the result
  bool connected = false;
  if (ssid.length() > 0) {
    connected = wlan_connectToWiFi(ssid, password);
  }

  if (connected) {
    wlan_saveMacAddress(WiFi.macAddress());
    wlan_stopAccessPoint();
    server.send(200, "text/html", "<h3>Saved and connected!</h3><a href='/'>Go back</a>");
  } else {
    server.send(200, "text/html", "<h3>Saved but failed to connect. Check credentials.</h3><a href='/'>Go back</a>");
  }
}

/**
 * @brief Initialize web server routes
 */
void wlan_init(void) {
  server.on("/", wlan_handleRoot);
  server.on("/save", wlan_handleSave);
  server.begin();
}
void wlan_clearPreferences() {
  prefs.begin("storage", false);
  prefs.clear();  // Deletes all keys in the namespace
  prefs.end();
  Serial.println("Preferences cleared!");
}
//mqtt stuff
void mqtt_init() {
  // Set server BEFORE connecting
  mqttClient.setServer(mqtt_server, mqtt_port);
  // Default-Socket-Timeout ist 15s — bei totem Broker wuerde ein einzelner
  // connect()-Versuch so lange blockieren. 5s reicht und bleibt loop-tauglich.
  mqttClient.setSocketTimeout(5);
  // WICHTIG: Default-Puffer ist nur 256 Bytes. Der Register-Payload
  // (thing_model + ble + capabilities + state) ist ~1 KB — PubSubClient
  // verwirft groessere publish() sonst stillschweigend → keine Registrierung.
  mqttClient.setBufferSize(2048);
  Serial.println("MQTT initialized");
}

void mqtt_setCallback(MqttCallbackFn cb) {
  mqttClient.setCallback(cb);
}

void mqtt_subscribeToTopics(void) {
  if (!mqttClient.connected()) {
    return;
  }

  String mac = wlan_loadMacAddress();

  String controlTopic = "devices/" + mac + "/control";
  String commandTopic = "devices/" + mac + "/command";
  String telemetryTopic = "devices/" + mac + "/telemetry";

  bool ok1 = mqttClient.subscribe(controlTopic.c_str());
  bool ok2 = mqttClient.subscribe(commandTopic.c_str());
  bool ok3 = mqttClient.subscribe(telemetryTopic.c_str());

  Serial.print("Subscribed control: ");
  Serial.println(ok1 ? "ok" : "failed");
  Serial.print("Subscribed command: ");
  Serial.println(ok2 ? "ok" : "failed");
  Serial.print("Subscribed telemetry: ");
  Serial.println(ok3 ? "ok" : "failed");
}

// Function to connect to MQTT broker
void reconnectMQTT() {
  String mac = wlan_loadMacAddress();
  String willTopic = "devices/" + mac + "/status";
  char lastWill[64];
  snprintf(lastWill, sizeof(lastWill), "{\"status\":\"offline\"}");
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiNotConnectedLog >= CONNECTION_LOG_INTERVAL_MS) {
      Serial.println("WiFi not connected!");
      lastWifiNotConnectedLog = millis();
    }
    return;
  }
  
  // Nur EIN Versuch pro Aufruf — nicht blockierend. Die Wiederholung mit
  // Backoff uebernimmt mqttloop() (alle CONNECTION_LOG_INTERVAL_MS).
  if (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print(" with user: ");
    Serial.println(mqtt_user);
    
    if (mqttClient.connect(mac.c_str(),
                       mqtt_user,
                       mqtt_password,
                       willTopic.c_str(),
                       1,       // QoS 1
                       true,    // retain the last will
                       lastWill)) {
  Serial.println("MQTT connected!");
  mqtt_subscribeToTopics();
  publishRegistration();
  
  const char* onlinePayload = "{\"status\":\"online\"}";
  if (mqttClient.publish(willTopic.c_str(), onlinePayload)) {
  Serial.println("Published online status (non-retained)");
  } else {
    Serial.println("Failed to publish online status");
  }

    
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      
      // Print what each code means
      switch(mqttClient.state()) {
        case -4: Serial.println(" (MQTT_CONNECTION_TIMEOUT)"); break;
        case -3: Serial.println(" (MQTT_CONNECTION_LOST)"); break;
        case -2: Serial.println(" (MQTT_CONNECT_FAILED)"); break;
        case -1: Serial.println(" (MQTT_DISCONNECTED)"); break;
        case 1: Serial.println(" (MQTT_CONNECT_BAD_PROTOCOL)"); break;
        case 2: Serial.println(" (MQTT_CONNECT_BAD_CLIENT_ID)"); break;
        case 3: Serial.println(" (MQTT_CONNECT_UNAVAILABLE)"); break;
        case 4: Serial.println(" (MQTT_CONNECT_BAD_CREDENTIALS)"); break;
        case 5: Serial.println(" (MQTT_CONNECT_UNAUTHORIZED)"); break;
      }
      
      Serial.println(" — retry via mqttloop()");
    }
  }
}
void publishdata(String capability,String value)
{ 
  wlan_setCapabilityValue(capability.c_str(), value.c_str());
  String mac = wlan_loadMacAddress();
  String data = "{\"data\":{\"" + capability + "\":" + value + "}}";
  String topic="devices/"+mac+"/telemetry";
  if (mqttClient.publish(topic.c_str(),data.c_str())) {
    Serial.print("Published to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(data);
  } else {
    Serial.println("Failed to publish");
    Serial.println(mqttClient.getWriteError());
  }
}

// Function to publish registration message
void publishRegistration() {
  String thingModelFragment = buildThingModelFragment();
  String payload = buildRegistrationPayload(getDeviceId(), thingModelFragment);
  String mac = wlan_loadMacAddress();
  String topic = "devices/register";
  
  if (mqttClient.publish(topic.c_str(), payload.c_str())) {
    Serial.print("Published to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);
  } else {
    Serial.println("Failed to publish");
    Serial.println(mqttClient.getWriteError());
  }
  // Publish an initial telemetry message containing all capabilities with empty values
  String telemetryTopic = "devices/" + mac + "/telemetry";
  buildCapabilitiesDocFromThingModel(thingModelFragment);
}


#define PREFS_KEY_MAC "mac_address"

void wlan_saveMacAddress(const String& mac) {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString(PREFS_KEY_MAC, mac);
  prefs.end();
}

String wlan_loadMacAddress() {
  prefs.begin(PREFS_NAMESPACE, true);
  String mac = prefs.getString(PREFS_KEY_MAC, "");
  prefs.end();
  return mac;
}

String wlan_getPairingCode() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();

  if (mac.length() < 4) {
    return "0000";
  }

  return mac.substring(mac.length() - 4);
}

String buildRegistrationPayload(const String& deviceId, const String& thingModelFragment) {
  // Load MAC from storage
  String mac = wlan_loadMacAddress();

  // Basic JSON assembly: device id, runtime mac, and thing_model fragment
  String payload = "{";
  payload += "\"device_id\":\"";
  payload += deviceId;
  payload += "\",";
  payload += "\"mac\":\"";
  payload += mac;
  payload += "\",";
  payload += thingModelFragment; // already includes "ble": { ... } and "thing_model": { ... }
  payload += "}";

  return payload;
}
void mqttloop()
{
 
  mqttClient.loop();
  // Reconnect + resubscribe if connection drops
  if (!mqttcheckconnection() && millis() - lastMqttReconnectAttempt >= CONNECTION_LOG_INTERVAL_MS) {
    lastMqttReconnectAttempt = millis();
    reconnectMQTT();
    if (mqttcheckconnection()) {
      mqtt_subscribeToTopics();
    }
  }
}
bool mqttcheckconnection()
{
return mqttClient.connected();
}


// Simple helper: is WiFi connected
bool wifiReady() {
  return WiFi.status() == WL_CONNECTED;
}
// Global docs (tune sizes to your model)
// Use StaticJsonDocument (suppress compatibility deprecation warnings)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static StaticJsonDocument<512> gCapabilitiesDoc;
static StaticJsonDocument<2048> gTmpParseDoc;
#pragma GCC diagnostic pop

// Build gCapabilitiesDoc from the thing model with empty string values
bool buildCapabilitiesDocFromThingModel(const String& thingModelFragment) {
  gCapabilitiesDoc.clear();
  gTmpParseDoc.clear();

  String src = "{" + thingModelFragment + "}";
  auto err = deserializeJson(gTmpParseDoc, src);
  if (err) return false;

  JsonArray caps = gTmpParseDoc["thing_model"]["capabilities"].as<JsonArray>();
  JsonObject root = gCapabilitiesDoc.to<JsonObject>();
  // Ensure 'data' object exists (use new API to create/convert)
  JsonObject data = root["data"].is<JsonObject>() ? root["data"].as<JsonObject>() : root["data"].to<JsonObject>();

  for (JsonObject cap : caps) {
    const char* id = cap["id"] | "";
    if (id[0]) data[id] = "";
  }
  return true;
}

// Write a string value into the capabilities doc (creates key if missing)
bool setCapabilityValue(JsonDocument& doc, const char* capability, const char* value) {
  JsonObject root = doc.as<JsonObject>();
  // Use new-style checks and to<JsonObject() to create if missing
  JsonObject data = root["data"].is<JsonObject>() ? root["data"].as<JsonObject>() : root["data"].to<JsonObject>();
  data[String(capability)] = value;
  return true;
}

// Read a capability value as a String (returns "" if missing)
String getCapabilityValue(JsonDocument& doc, const char* capability) {
  JsonObject root = doc.as<JsonObject>();
  if (root["data"].isNull()) return "";
  JsonVariant v = root["data"][capability];
  if (!v) return "";
  if (v.is<const char*>()) return String(v.as<const char*>());
  String tmp; serializeJson(v, tmp); return tmp;
}
// Wrappers that use the internal global gCapabilitiesDoc
bool wlan_setCapabilityValue(const char* capability, const char* value) {
  return setCapabilityValue(gCapabilitiesDoc, capability, value);
}

String wlan_getCapabilityValue(const char* capability) {
  return getCapabilityValue(gCapabilitiesDoc, capability);
}
