#include "connecthub.h"
#include <Arduino.h>
#include <Actionconnectlibrary.h>   // zieht Wlan.h + BLEManager.h mit

static measure_callback_t s_measure_cb = nullptr;

// Eingehende Commands (MQTT devices/<mac>/command|control oder BLE-Write).
// Wir werten nur die "measure"-Capability aus (IMPULSE, writeable).
static void on_transport_message(const char *source, const char *topic, const String &payload)
{
    Serial.printf("[hub] <%s> %s\n", source ? source : "?", payload.c_str());

    String capability = connectHubExtractJsonValue(payload, "capability");
    capability.trim();

    if (capability == "measure") {
        if (s_measure_cb) s_measure_cb();
    }
}

void connect_hub_begin(const char *ssid,      const char *password,
                       const char *mqtt_host, uint16_t    mqtt_port,
                       const char *mqtt_user, const char *mqtt_pass,
                       measure_callback_t on_measure)
{
    (void)ssid; (void)password;
    (void)mqtt_host; (void)mqtt_port; (void)mqtt_user; (void)mqtt_pass;

    s_measure_cb = on_measure;

    connectHubSetReceiveCallback(on_transport_message);
    connectHubInit();   // WiFi(+Provisioning) + MQTT + BLE, registriert ThingModel

    // Vorgabe der Team-Repo: Pairing-Code muss im Serial-Monitor erscheinen.
    Serial.printf("[hub] Pairing Code: %s\n", wlan_getPairingCode().c_str());
}

void connect_hub_loop(void)
{
    server.handleClient();          // AP-WebConfig (Provisioning-Fallback)
    connectHubProcessTransport();   // MQTT + BLE am Leben halten
}

void connect_hub_publish_telemetry(uint8_t moisture, bool pump_on, bool tank_full)
{
    if (!wifiReady() || !mqttcheckconnection()) return;

    // Format wie bisher: {"data":{"moisture":..,"pump":"ON","tank_status":"OFF"}}
    String payload = "{\"data\":{";
    payload += "\"moisture\":" + String(moisture);
    payload += ",\"pump\":\"";        payload += pump_on   ? "ON" : "OFF"; payload += "\"";
    payload += ",\"tank_status\":\""; payload += tank_full ? "ON" : "OFF"; payload += "\"";
    payload += "}}";

    publish_telemetry(payload);

    // interne Capability-DB der Bibliothek konsistent halten
    connectHubSetCapability("moisture",    String(moisture).c_str());
    connectHubSetCapability("pump",        pump_on   ? "ON" : "OFF");
    connectHubSetCapability("tank_status", tank_full ? "ON" : "OFF");
}

bool connect_hub_is_connected(void)
{
    return wifiReady() && mqttcheckconnection();
}
