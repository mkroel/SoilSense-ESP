#pragma once

#include <stdint.h>
#include <stdbool.h>

// Callback-Typ für On-Demand-Messung (via MQTT-Command)
typedef void (*measure_callback_t)(void);

/**
 * WiFi + MQTT initialisieren, Thing-Model registrieren.
 * on_measure wird aufgerufen wenn Hub eine Messung anfordert.
 */
void connect_hub_begin(const char *ssid,      const char *password,
                       const char *mqtt_host, uint16_t    mqtt_port,
                       const char *mqtt_user, const char *mqtt_pass,
                       measure_callback_t on_measure);

/**
 * In jedem loop()-Durchlauf aufrufen.
 * Hält MQTT am Leben, verarbeitet eingehende Commands.
 */
void connect_hub_loop(void);

/**
 * Sendet nur geänderte Werte als Telemetrie.
 * Kein Publish wenn sich nichts geändert hat.
 */
void connect_hub_publish_telemetry(uint8_t moisture, bool pump_on, bool tank_full);

/** true wenn WiFi + MQTT beide verbunden sind */
bool connect_hub_is_connected(void);