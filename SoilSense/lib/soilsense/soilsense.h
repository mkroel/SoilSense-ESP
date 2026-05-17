#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void soil_sense_init(void);
void soil_sense_update(bool pump_state);

uint16_t soil_sense_get_raw_value(void);
uint8_t  soil_sense_get_moisture_percent(void);
const char* soil_sense_get_moisture_status(void);

void soil_sense_power_on(void);
void soil_sense_power_off(void);

#ifdef __cplusplus
}
#endif