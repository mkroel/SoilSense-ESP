#include "soilctrl.h"
#include "config.h"
#include <Arduino.h>
#include <stdio.h>

static PumpState pump_state = PUMP_OFF;
static TankState tank_state = TANK_EMPTY;

uint32_t last_pump_toggle_time = 0;
uint32_t pump_start_time = 0;
uint32_t pump_stop_time = 0;

void soil_ctrl_init(void) {
    digitalWrite(PUMP_PIN, PUMP_OFF_LEVEL); // Ensure pump is off
    // Open-Drain wie STM32 (GPIO_MODE_OUTPUT_OD) — externer Pullup nötig
    pinMode(PUMP_PIN, OUTPUT_OPEN_DRAIN);
    pinMode(FLOAT_PIN, INPUT_PULLUP);

    if (digitalRead(FLOAT_PIN) == FLOAT_FULL) {
        tank_state = TANK_FULL; // top
    } else {
        tank_state = TANK_EMPTY; // bottom
    }
    
    // undefined state -> safe state
    soil_ctrl_immediate_stop();
    pump_stop_time = 0;
}

void soil_ctrl_update(void) {
    int float_raw = digitalRead(FLOAT_PIN);
    TankState new_tank = (float_raw == FLOAT_FULL) ? TANK_FULL : TANK_EMPTY;
    if (new_tank != tank_state) {
        printf("[ctrl] Tank %s (FLOAT_PIN=%d)\n",
               new_tank == TANK_FULL ? "FULL" : "EMPTY", float_raw);
    }
    tank_state = new_tank;
    if (tank_state == TANK_EMPTY && pump_state == PUMP_ON) {
        soil_ctrl_immediate_stop();
    }

    if (pump_state == PUMP_ON) {
        if ((millis() - pump_start_time) > PUMP_MAX_RUNTIME) {
            soil_ctrl_immediate_stop(); // Timeout
        }
    }
} 

void soil_ctrl_set_pump(bool turn_on) {
    uint32_t now = millis();

    if (turn_on) {
        if (tank_state == TANK_EMPTY) {
            return; 
        }
        if (pump_state == PUMP_OFF && (now - pump_stop_time >= PUMP_COOLDOWN || pump_stop_time == 0)) {
            digitalWrite(PUMP_PIN, PUMP_ON_LEVEL); // On
            pump_state = PUMP_ON;
            pump_start_time = now;
            last_pump_toggle_time = now;
            printf("[ctrl] PUMP ON\n");
            delay(100);
        }
    } else {
        if (pump_state == PUMP_ON) {
            soil_ctrl_immediate_stop();
            
            last_pump_toggle_time = now;
        }
    }
}

void soil_ctrl_immediate_stop(void) {
    digitalWrite(PUMP_PIN, PUMP_OFF_LEVEL); // Off
    if (pump_state == PUMP_ON) printf("[ctrl] PUMP OFF\n");
    pump_state = PUMP_OFF;
    pump_stop_time = millis();
}

bool soil_ctrl_is_pump_on(void) {
    return (pump_state == PUMP_ON);
}

bool soil_ctrl_is_tank_empty(void) {
    return (tank_state == TANK_EMPTY);
}

int soil_ctrl_get_status(void) {
    switch (tank_state)
    {
    case TANK_EMPTY:
        return 2; // Tank empty
    
    case TANK_FULL:
        if (pump_state == PUMP_ON) {
            return 1; // Pumpe active
        } else {
            return 0; // Pumpe off, Tank full
        }

    default:
        return 0;
    }
}