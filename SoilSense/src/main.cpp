#include <Arduino.h>
#include "config.h"
#include "soilctrl.h"
#include "soilsense.h"
#include "visualize.h"
#include <esp_task_wdt.h>

static uint32_t last_measurement = 0;
static uint32_t measurement_interval = MEASURE_INTERVAL_ACTIVE;
static uint8_t tank_empty_counter = 0;
static bool is_watering = false;

void setup() {
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL); 
  Serial.begin(115200);
  visualize_init();
  soil_sense_init();
  soil_ctrl_init();
  Serial.println("SoilSense ESP32 boot");
}

void loop() {
  soil_ctrl_update();

  if (millis() - last_measurement >= measurement_interval) {
    soil_sense_power_on();
    delay(50);
    soil_sense_update(soil_ctrl_is_pump_on());
    soil_sense_power_off();

    uint8_t moisture = soil_sense_get_moisture_percent();
    if (moisture <= MOISTURE_THRESHOLD_OPTIMAL) is_watering = true;
    if (moisture >= MOISTURE_THRESHOLD_STOP)    is_watering = false;

    if (!soil_ctrl_is_tank_empty()) {
      if (is_watering) {
        soil_ctrl_set_pump(true);
        measurement_interval = MEASURE_INTERVAL_ACTIVE;
      } else {
        soil_ctrl_set_pump(false);
        measurement_interval = MEASURE_INTERVAL_SLEEP;
      }
    } else {
      soil_ctrl_set_pump(false);
      tank_empty_counter++;
      if (tank_empty_counter >= TANK_EMPTY_COUNTER_MAX) {
        measurement_interval = MEASURE_INTERVAL_SLEEP;
        tank_empty_counter = 0;
      }
    }

    last_measurement = millis();
  }

  visualize_update(soil_ctrl_get_status());
  esp_task_wdt_reset();
  delay(100);
}