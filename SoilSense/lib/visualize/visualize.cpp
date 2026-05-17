#include "visualize.h"
#include "config.h"
#include <Arduino.h>

void visualize_Init(void)
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void visualize_Update(int status)
{
    unsigned long now = millis();
    bool ledOn = false;

    switch (status)
    {
    case 0: // ok
        ledOn = false;
        break;

    case 1: //pump active:
        ledOn = true;
        break;
    
    case 2: //tank empty:
        ledOn = (now / 500) % 2 == 0;
        break;

    case 3:  // disconnected
        ledOn = (now % 2000) < 80 || ((now % 2000) > 200 && (now % 2000) < 280);
        break;
    
    default:
        ledOn = (now % 1500) < 700 || ((now % 1500) > 850 && (now % 1500) < 1000);
        break;
    }

    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
}