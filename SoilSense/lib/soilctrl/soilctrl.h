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

void soil_ctrl_init(void);
void soil_ctrl_update(void);
void soil_ctrl_set_pump(bool turn_on);
void soil_ctrl_immediate_stop(void);
bool soil_ctrl_is_pump_on(void);
bool soil_ctrl_is_tank_empty(void);
int  soil_ctrl_get_status(void);

#ifdef __cplusplus
}
#endif