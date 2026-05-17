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
#define FLOAT_FULL        LOW    // Schwimmer oben
#define PUMP_ON_LEVEL     LOW    // active-LOW Relais
#define PUMP_OFF_LEVEL    HIGH

// Timer & Intervalle
#define MEASURE_INTERVAL_SLEEP   (3UL * 60 * 60 * 1000)  // 3 Stunden Idle
#define MEASURE_INTERVAL_ACTIVE  (20 * 1000)              // 20s Aufweckfenster
#define ACTIVE_WINDOW            (90 * 1000)              // 90s aktives Messfenster nach Wake

#define PUMP_MAX_RUNTIME        (5 * 1000)      // Aus nach 5s
#define PUMP_COOLDOWN           (20 * 1000)     // 30s Pause nach Pumpenende

// Threshholds
// Erde-kalibriert (nicht Wasserglas-Extreme):
//   trockene Erde ~ 2400 mV → 0 %
//   nasse Erde    ~ 1300 mV → ~100 %
// In-soil readings ohne reines Wasser sind realistischer als Luft/Glas-Extreme.
#define THRESHOLD_DRY           2400   // ab hier 0 %
#define THRESHOLD_WET           1300   // ab hier 100 %
#define PUMP_OFFSET_CORRECTION  +20    // Pumpe drückt Messung ~20mV nach unten → addieren

#define MOISTURE_THRESHOLD_DRY      25 // Prozent
#define MOISTURE_THRESHOLD_OPTIMAL  40
#define MOISTURE_THRESHOLD_STOP     60 // Pumpe stoppt erst bei 60%
#define MOISTURE_THRESHOLD_WET      75

// Messungen
#define MEASUREMENT_SAMPLES     64
#define TANK_EMPTY_COUNTER_MAX  60 // Anzahl Messungen mit leerem Tank bis Idle