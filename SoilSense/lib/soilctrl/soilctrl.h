#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PUMP_OFF = 0,
    PUMP_ON  = 1
} PumpState;

typedef enum {
    TANK_EMPTY = 0,
    TANK_FULL  = 1
} TankState;

#ifdef __cplusplus
extern "C" {
#endif

void SoilCtrl_Init(void);
void SoilCtrl_Update(void);
void SoilCtrl_SetPump(bool turnOn);
void SoilCtrl_ImmediateStop(void);
bool SoilCtrl_IsPumpOn(void);
bool SoilCtrl_IsTankEmpty(void);
int  SoilCtrl_getStatus(void);

#ifdef __cplusplus
}
#endif