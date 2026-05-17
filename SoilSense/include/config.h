#pragma once

// Hardware-Pins
#define PUMP_PIN          27
#define FLOAT_PIN         33
#define SENSOR1_AOUT_PIN  34
#define SENSOR2_AOUT_PIN  35
#define SENSOR1_VCC_PIN   25
#define SENSOR2_VCC_PIN   26
#define LED_PIN            2

// Logik-Defines
#define FLOAT_FULL        HIGH    // Schwimmer oben = HIGH (oder LOW, je nach Verdrahtung)
#define PUMP_ON_LEVEL     LOW     // dein Relais war active-low (GPIO_PIN_RESET = AN)

// Timer & Intervalle
#define MEASURE_INTERVAL_SLEEP   (3UL * 60 * 60 * 1000) // 3 Stunden
#define MEASURE_INTERVAL_ACTIVE  (3 * 1000)        // 5 Sekunden

#define PUMP_MAX_RUNTIME        (5 * 1000)      // Aus nach 5s
#define PUMP_COOLDOWN           (20 * 1000)     // 30s Pause nach Pumpenende

// Threshholds
#define THRESHOLD_DRY           3200
#define THRESHOLD_WET           1300
#define PUMP_OFFSET_CORRECTION  -85 // Abweichung durch Pumpenlast (111 bei Ventilator, -85 bei der Pumpe mit 3,3V Versorgung, -450 bei 5V)

#define MOISTURE_THRESHOLD_DRY      25 // Prozent
#define MOISTURE_THRESHOLD_OPTIMAL  40
#define MOISTURE_THRESHOLD_STOP     60 // Pumpe stoppt erst bei 60%
#define MOISTURE_THRESHOLD_WET      75

// Messungen
#define MEASUREMENT_SAMPLES     64
#define TANK_EMPTY_COUNTER_MAX  60 // Anzahl Messungen mit leerem Tank bis Idle

// Hardwareadressen
#define LCD_ADDR       (0x4E) //LCD Displays