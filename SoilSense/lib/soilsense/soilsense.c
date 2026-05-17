#include "soilsense.h"
#include "config.h"
#include <Arduino.h>
#include <stdio.h>

// vars
static uint16_t moisture_raw = 0;
static uint8_t moisture_percent = 0;

void soil_sense_init(void) 
{
    pinMode(SENSOR1_VCC_PIN, OUTPUT);
    pinMode(SENSOR2_VCC_PIN, OUTPUT);
    digitalWrite(SENSOR1_VCC_PIN, LOW);
    digitalWrite(SENSOR2_VCC_PIN, LOW);
}

void soil_sense_update(bool pump_state) 
{
    uint32_t sum1 = 0, sum2 = 0;

    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        sum1 += analogReadMilliVolts(SENSOR1_AOUT_PIN);
        sum2 += analogReadMilliVolts(SENSOR2_AOUT_PIN);
    }

    uint16_t avg1 = sum1 / MEASUREMENT_SAMPLES;
    uint16_t avg2 = sum2 / MEASUREMENT_SAMPLES;
    moisture_raw = (avg1 + avg2) / 2;

    printf("[sense] S1=%u mV  S2=%u mV  avg=%u mV  pump=%d\n",
                  avg1, avg2, moisture_raw, pump_state);

    if (pump_state) {
        int32_t corrected = (int32_t)moisture_raw + PUMP_OFFSET_CORRECTION;
        if (corrected < 0) corrected = 0;
        moisture_raw = (uint16_t)corrected;
    }

    if (moisture_raw >= THRESHOLD_DRY) moisture_percent = 0;
    else if (moisture_raw <= THRESHOLD_WET) moisture_percent = 100;
    else {
        int32_t range = THRESHOLD_WET - THRESHOLD_DRY;
        int32_t value = moisture_raw - THRESHOLD_DRY;
        moisture_percent = (uint8_t)((value * 100) / range);
    }

    printf("[sense] raw=%u → %u%% (%s)\n",
                  moisture_raw, moisture_percent, soil_sense_get_moisture_status());
}

uint16_t soil_sense_get_raw_value(void) {
    return moisture_raw;
}

uint8_t soil_sense_get_moisture_percent(void) {
    return moisture_percent;
}

const char *soil_sense_get_moisture_status(void)
{
    if (moisture_percent < MOISTURE_THRESHOLD_DRY) {
        return "Sehr trocken";
    } else if (moisture_percent < MOISTURE_THRESHOLD_OPTIMAL) {
        return "Trocken";
    } else if (moisture_percent < MOISTURE_THRESHOLD_WET) {
        return "Optimal";
    } else {
        return "Zu nass";
    }
}

void soil_sense_power_on(void) {
    digitalWrite(SENSOR1_VCC_PIN, HIGH);
    digitalWrite(SENSOR2_VCC_PIN, HIGH);
    delay(50);
}

void soil_sense_power_off(void) {
    digitalWrite(SENSOR1_VCC_PIN, LOW);
    digitalWrite(SENSOR2_VCC_PIN, LOW);
}