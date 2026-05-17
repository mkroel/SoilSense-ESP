#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void SoilSense_Init(void);
void SoilSense_Update(bool pumpState);

uint16_t SoilSense_GetRawValue(void);
uint8_t SoilSense_GetMoisturePercent(void);
const char* SoilSense_GetMoistureStatus(void);

void SoilSense_PowerOn(void);
void SoilSense_PowerOff(void);

#ifdef __cplusplus
}
#endif