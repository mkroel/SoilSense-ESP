#include <Arduino.h>
#include "config.h"
#include "secrets.h"
#include "soilctrl.h"
#include "soilsense.h"
#include "connecthub.h"
#include "visualize.h"
#include <esp_task_wdt.h>

static uint32_t last_measurement = 0;
static uint32_t measurement_interval = MEASURE_INTERVAL_ACTIVE;
static uint32_t active_window_start = 0;   // Beginn des aktuellen Messfensters
static uint8_t tank_empty_counter = 0;
static bool is_watering = false;
static bool force_measurement = false;

void on_measure_triggered() {
  force_measurement = true;
}

void setup() {
  Serial.begin(115200);
  visualize_init();
  soil_sense_init();
  soil_ctrl_init();
  Serial.println("SoilSense ESP32 boot");

  connect_hub_begin(
    WIFI_SSID, WIFI_PASSWORD,
    MQTT_HOST, MQTT_PORT,
    MQTT_USER, MQTT_PASSWORD,
    on_measure_triggered
  );

  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL);
}

void loop() {
  connect_hub_loop();
  soil_ctrl_update();

  if (millis() - last_measurement >= measurement_interval || force_measurement) {
    // Wake-Detection: erste Messung nach Sleep startet neues aktives Fenster
    bool waking_from_sleep = (measurement_interval == MEASURE_INTERVAL_SLEEP);
    if (waking_from_sleep || force_measurement) {
      active_window_start = millis();
      Serial.println("[main] === Active window started ===");
    }

    soil_sense_power_on();
    delay(50);
    soil_sense_update(soil_ctrl_is_pump_on());
    soil_sense_power_off();
    force_measurement = false;

    uint8_t moisture = soil_sense_get_moisture_percent();
    if (moisture <= MOISTURE_THRESHOLD_OPTIMAL) is_watering = true;
    if (moisture >= MOISTURE_THRESHOLD_STOP)    is_watering = false;

    if (!soil_ctrl_is_tank_empty()) {
      if (is_watering) {
        soil_ctrl_set_pump(true);
        measurement_interval = MEASURE_INTERVAL_ACTIVE;
        active_window_start = millis();   // Fenster verlängern solange gepumpt wird
      } else {
        soil_ctrl_set_pump(false);
        // Im aktiven Fenster bleiben → erst nach ACTIVE_WINDOW schlafen gehen
        if (millis() - active_window_start < ACTIVE_WINDOW) {
          measurement_interval = MEASURE_INTERVAL_ACTIVE;
        } else {
          measurement_interval = MEASURE_INTERVAL_SLEEP;
        }
      }
    } else {
      soil_ctrl_set_pump(false);
      tank_empty_counter++;
      if (tank_empty_counter >= TANK_EMPTY_COUNTER_MAX) {
        measurement_interval = MEASURE_INTERVAL_SLEEP;
        tank_empty_counter = 0;
      }
    }


    // Telemetrie an Hub schicken (Delta-only, intern gefiltert)
    connect_hub_publish_telemetry(
      moisture,
      soil_ctrl_is_pump_on(),
      !soil_ctrl_is_tank_empty()   // tank_status: true = voll
    );

    Serial.printf("[main] moisture=%u%% watering=%d tank_empty=%d pump=%d interval=%lums\n",
                  moisture, is_watering,
                  soil_ctrl_is_tank_empty(), soil_ctrl_is_pump_on(),
                  measurement_interval);

    last_measurement = millis();
  }

  uint8_t status = soil_ctrl_get_status();
  if (!connect_hub_is_connected()) status = 3;
  visualize_update(status);
  esp_task_wdt_reset();
  delay(100);
}