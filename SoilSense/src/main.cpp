#include <Arduino.h>
#include "config.h"
#include "soilctrl.h"
#include "visualize.h"

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("SoilSense ESP32 boot");
  visualize_Init();
}

void loop() {
  for (int i = 0; i <= 2; i++) {
    Serial.printf("status = %d\n", i);
    unsigned long start = millis();
    while (millis() - start < 1000 * 30) {
      visualize_Update(i);
      delay(20);
    }
  }
}