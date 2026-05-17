# SoilSense ESP32

Automatische Pflanzenbewässerung auf Basis eines ESP32, portiert von STM32 (Embedded Systems 1, FAU). Meldet Sensordaten via MQTT an [Connect-Hub](https://hub.finishing2026.sw2e-lab.de).

## Features

- Kapazitive Bodenfeuchtesensoren (2×, gemittelt)
- Automatische Pumpensteuerung mit Hysterese
- Schwimmschalter-Absicherung (Tank-leer-Erkennung)
- Pump-Timeout + Cooldown gegen Dauerläufe
- 3h Sleep-Modus wenn Feuchte optimal
- MQTT-Telemetrie an Connect-Hub (nur geänderte Werte)
- On-Demand-Messung via Hub (IMPULSE-Command)
- LED-Statusanzeige (aus / dauerhaft an / blinken / disconnect)
- Watchdog + Brownout Detection

## Hardware

| Bauteil | Beschreibung |
|---|---|
| ESP32 NodeMCU | USB-C, CH340, WROOM-32 |
| 2× kapazitiver Feuchtigkeitssensor | 3-Pin (VCC, GND, AOUT) |
| Relais-Modul | 5V, active-low |
| Schwimmschalter | Tank-leer-Erkennung |
| Pumpe | 5V, extern versorgt |

## Pin-Mapping

| GPIO | Funktion |
|---|---|
| 34 | Sensor 1 AOUT (ADC1) |
| 35 | Sensor 2 AOUT (ADC1) |
| 25 | Sensor 1 VCC |
| 26 | Sensor 2 VCC |
| 27 | Relais IN |
| 33 | Schwimmschalter |
| 2 | Status-LED (onboard) |

## Projektstruktur

```
SoilSense/
├── include/
│   ├── config.h        # Pin-Mapping, Schwellwerte, Intervalle
│   └── secrets.h       # WiFi + MQTT Credentials (nicht im Repo)
├── lib/
│   ├── soilsense/      # ADC-Messung, Feuchte-Berechnung
│   ├── soilctrl/       # Pumpensteuerung, Tank-Logik
│   ├── connecthub/     # WiFi, MQTT, Thing-Model, Telemetrie
│   └── visualize/      # LED-Statusanzeige
└── src/
    └── main.cpp        # setup/loop, State-Machine
```

## Konfiguration

Pins und Schwellwerte in `include/config.h`. Credentials in `include/secrets.h` anlegen (wird nicht versioniert):

```cpp
#pragma once

#define WIFI_SSID     "..."
#define WIFI_PASSWORD "..."

#define MQTT_HOST     "..."
#define MQTT_PORT     1883
#define MQTT_USER     "..."
#define MQTT_PASSWORD "..."
```

## Connect-Hub Integration

Das Gerät registriert sich beim Boot automatisch unter `devices/register` mit folgendem Thing Model:

| Capability | Typ | Richtung |
|---|---|---|
| `moisture` | NUMBER (%) | readable |
| `pump` | ON_OFF | readable |
| `tank_status` | ON_OFF | readable |
| `measure` | IMPULSE | writable |

Telemetrie wird auf `devices/<MAC>/telemetry` published — nur bei Änderungen. Status (online/offline) wird retained auf `devices/<MAC>/status` gesetzt.

## Build

```bash
pio run              # kompilieren
pio run --target upload   # flashen
pio device monitor   # Serial Monitor (115200 baud)
```

## Lizenz

MIT — siehe [LICENSE](LICENSE)
