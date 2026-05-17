#include "connecthub.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RECONNECT_INTERVAL  5000   // ms zwischen MQTT-Reconnect-Versuchen
#define WIFI_TIMEOUT        10000  // ms warten auf WiFi beim Start

static WiFiClient        wifi_client;
static PubSubClient      mqtt_client(wifi_client);
static measure_callback_t measure_cb = nullptr;

static String  mac;
static String  topic_telemetry;
static String  topic_status;
static String  topic_command;

static const char *s_mqtt_user;
static const char *s_mqtt_pass;

static uint32_t last_reconnect_attempt = 0;

// Delta-Tracking — 255/-1 = noch nie gesendet
static uint8_t last_moisture  = 255;
static int8_t  last_pump_on   = -1;
static int8_t  last_tank_full = -1;

static String build_thing_model()
{
    JsonDocument doc;

    doc["device_id"]            = "SoilSense";
    doc["mac"]                  = mac;
    doc["metadata"]["label"]    = "Watering";
    doc["metadata"]["category"] = "Plants";

    JsonArray caps = doc["capabilities"].to<JsonArray>();

    JsonObject moisture = caps.add<JsonObject>();
    moisture["id"]        = "moisture";
    moisture["type"]      = "NUMBER";
    moisture["direction"] = "readable";
    moisture["label"]     = "Feuchte";
    moisture["unit"]      = "%";
    moisture["min"]       = 0;
    moisture["max"]       = 100;
    moisture["featured"]  = true;

    JsonObject pump = caps.add<JsonObject>();
    pump["id"]        = "pump";
    pump["type"]      = "ON_OFF";
    pump["direction"] = "readable";
    pump["label"]     = "Pumpe";
    pump["featured"]  = true;

    JsonObject tank = caps.add<JsonObject>();
    tank["id"]        = "tank_status";
    tank["type"]      = "ON_OFF";
    tank["direction"] = "readable";
    tank["label"]     = "Tank";
    tank["featured"]  = true;

    JsonObject measure = caps.add<JsonObject>();
    measure["id"]        = "measure";
    measure["type"]      = "IMPULSE";
    measure["direction"] = "writable";
    measure["label"]     = "Messen";

    String out;
    serializeJson(doc, out);
    return out;
}

static void on_mqtt_message(const char *topic, byte *payload, unsigned int length)
{
    // nur Commands auf unserem Topic verarbeiten
    if (String(topic) != topic_command) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) return;

    const char *capability = doc["capability"];
    if (capability && String(capability) == "measure") {
        if (measure_cb != nullptr) measure_cb();
    }
}

static void mqtt_connect()
{
    if (mqtt_client.connected()) return;

    String client_id = "soilsense-" + mac;

    bool ok = mqtt_client.connect(
        client_id.c_str(),
        s_mqtt_user,
        s_mqtt_pass,
        topic_status.c_str(),   // Last-Will Topic
        1,                       // QoS
        true,                    // retained
        "offline"
    );

    if (!ok) {
        last_reconnect_attempt = millis();
        Serial.printf("[hub] MQTT connect failed, rc=%d\n", mqtt_client.state());
        return;
    }

    Serial.println("[hub] MQTT connected");

    // Status online setzen
    mqtt_client.publish(topic_status.c_str(), "online", true);

    // Gerät registrieren
    String tm = build_thing_model();
    mqtt_client.publish("devices/register", tm.c_str(), false);
    Serial.println("[hub] Thing model published");

    // Command-Topic subscriben
    mqtt_client.subscribe(topic_command.c_str());
    Serial.printf("[hub] Subscribed to %s\n", topic_command.c_str());

    // Delta-Cache zurücksetzen → nächstes Telemetry-Publish sendet alles
    last_moisture  = 255;
    last_pump_on   = -1;
    last_tank_full = -1;
}

void connect_hub_begin(const char *ssid,      const char *password,
                       const char *mqtt_host, uint16_t    mqtt_port,
                       const char *mqtt_user, const char *mqtt_pass,
                       measure_callback_t on_measure)
{
    measure_cb   = on_measure;
    s_mqtt_user  = mqtt_user;
    s_mqtt_pass  = mqtt_pass;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // MAC ist direkt nach begin() verfügbar
    mac             = WiFi.macAddress();
    topic_telemetry = "devices/" + mac + "/telemetry";
    topic_status    = "devices/" + mac + "/status";
    topic_command   = "devices/" + mac + "/command";

    Serial.printf("[hub] MAC: %s\n", mac.c_str());

    // kurz auf WiFi warten (blockierend, nur beim Boot)
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[hub] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[hub] WiFi timeout — will retry in loop");
    }

    mqtt_client.setServer(mqtt_host, mqtt_port);
    mqtt_client.setCallback(on_mqtt_message);
    mqtt_client.setKeepAlive(30);
    mqtt_client.setBufferSize(1024);  // Thing-Model kann groß werden

    mqtt_connect();
}

void connect_hub_loop(void)
{
    // WiFi weg → reconnect, MQTT macht keinen Sinn
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
        return;
    }

    // MQTT weg → reconnect mit Backoff
    if (!mqtt_client.connected()) {
        if (millis() - last_reconnect_attempt >= RECONNECT_INTERVAL) {
            mqtt_connect();
        }
        return;
    }

    mqtt_client.loop();
}

void connect_hub_publish_telemetry(uint8_t moisture, bool pump_on, bool tank_full)
{
    if (!mqtt_client.connected()) return;

    JsonDocument doc;
    JsonObject data = doc["data"].to<JsonObject>();

    if (moisture != last_moisture) {
        data["moisture"]   = moisture;
        last_moisture      = moisture;
    }
    if ((int8_t)pump_on != last_pump_on) {
        data["pump"]       = pump_on ? "ON" : "OFF";
        last_pump_on       = (int8_t)pump_on;
    }
    if ((int8_t)tank_full != last_tank_full) {
        data["tank_status"] = tank_full ? "ON" : "OFF";
        last_tank_full      = (int8_t)tank_full;
    }

    if (data.size() == 0) return;  // nichts geändert

    String out;
    serializeJson(doc, out);
    mqtt_client.publish(topic_telemetry.c_str(), out.c_str());
    Serial.printf("[hub] Telemetry: %s\n", out.c_str());
}

bool connect_hub_is_connected(void)
{
    return WiFi.status() == WL_CONNECTED && mqtt_client.connected();
}
