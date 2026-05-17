# SoilSense ESP32 — Pinout & Verdrahtung

## ESP32 NodeMCU (38-Pin, USB-C oben)

```bash
                    ┌──────────────┐
               USB-C│              │
                    │   ESP32      │
              3V3 ──┤ 3V3      VIN ├── 5V Rail → Relais VCC
              GND ──┤ GND      GND ├── GND Rail
                    ┤ D15      D13 ├
          LED ★ ───┤ D2       D12 ├
                    ┤ D4       D14 ├
                    ┤ RX2      D27 ├── PUMP_PIN      → Relais IN
                    ┤ TX2      D26 ├── SENSOR2_VCC   → Sensor 2 VCC
                    ┤ D5       D25 ├── SENSOR1_VCC   → Sensor 1 VCC
                    ┤ D18      D33 ├── FLOAT_PIN     → Schwimmschalter
                    ┤ D19      D32 ├
                    ┤ D21      D35 ├── SENSOR2_AOUT  ← Sensor 2 Signal
                    ┤ RX0      D34 ├── SENSOR1_AOUT  ← Sensor 1 Signal
                    ┤ TX0      D39 ├
                    ┤ D22      D36 ├
                    ┤ D23       EN ├── Reset (nicht verwenden)
                    │              │
                    └──────────────┘

★ D2 = onboard blaue LED (kein externer Anschluss nötig)
← = Input (lesen)
→ = Output (schalten/steuern)
```

## Breadboard-Layout

```bash
                         ┌─────────────────────────────────────────────────────┐
  + Rail (5V) ══════════╪═════════════════════════════════════════════════════╪═ 5V Netzteil +
  - Rail (GND) ═════════╪═════════════════════════════════════════════════════╪═ 5V Netzteil -
                         │                                                     │
      Zeile a–e          │   ┌─────────────────────┐                          │
                         │   │       ESP32          │                          │
                         │   │   (überspannt Mitte) │                          │
      Zeile f–j          │   └─────────────────────┘                          │
                         │                                                     │
  - Rail (GND) ═══════════════════════════════════════════════════════════════ GND Rail
  + Rail (5V)  ══ unbenutzt (Pins auf dieser Seite optional)                  │
                         └─────────────────────────────────────────────────────┘
```

## Verdrahtung im Detail

### Relais (neben Breadboard, 10cm M→F Kabel)

```bash
Relais-Seite (male Pins)    Kabel           Breadboard
────────────────────────    ──────────      ──────────────────
VCC                    ─── M→F 10cm ──→   5V Rail
GND                    ─── M→F 10cm ──→   GND Rail
IN                     ─── M→F 10cm ──→   GPIO27 (D27)

Relais-Seite (female Klemmen)
────────────────────────────────────────────────────────
COM ─── 5V Netzteil +
NO  ─── Pumpe +          (Pumpe male direkt in Klemme)
        Pumpe -  ──────── 5V Netzteil -  (gemeinsame GND)
```

### Sensoren (50cm M→M Kabel, Sensorende female)

```bash
Sensor 1                    50cm M→M        Breadboard-Reihe
────────────────────────    ──────────      ──────────────────
VCC (female)           ←── M→M         ─── GPIO25 (D25)
GND (female)           ←── M→M         ─── GND Rail
AOUT (female)          ──→ M→M         ─── GPIO34 (D34)

Sensor 2
────────────────────────
VCC (female)           ←── M→M         ─── GPIO26 (D26)
GND (female)           ←── M→M         ─── GND Rail
AOUT (female)          ──→ M→M         ─── GPIO35 (D35)
```

### Schwimmschalter (male Kabel, weit weg am Tank)

```bash
Empfehlung: Mini-Breadboard am Tank als Junction

Mini-BB am Tank:              50cm M→M        Haupt-Breadboard
─────────────────────────     ──────────      ──────────────────
Reihe X: Switch-Pin 1 (male) + M→M-Ende ─── GPIO33 (D33)
Reihe Y: Switch-Pin 2 (male) + M→M-Ende ─── GND Rail

→ beide males in dieselbe Mini-BB-Reihe = elektrisch verbunden
```

### Stromversorgung

```bash
USB-C ──→ ESP32 (5V intern → 3.3V Regler)
       └─→ 5V Rail (Breadboard) ──→ Relais VCC

Externes 5V Netzteil ──→ Relais COM + Pumpe GND
(NICHT aus USB!)         gemeinsame GND mit System verbinden!
```

## Kabel-Übersicht

| Verbindung | Kabeltyp | Anzahl |
|---|---|---|
| Relais → Breadboard | 10cm M→F | 3 |
| Sensor 1 → Breadboard | 50cm M→M | 3 |
| Sensor 2 → Breadboard | 50cm M→M | 3 |
| Schwimmschalter → Mini-BB | direkt (male) | 2 |
| Mini-BB → Breadboard | 50cm M→M | 2 |
| Pumpe → Relais-Klemme | direkt (male) | 2 |

## Wichtige Hinweise

- **Pumpe-GND** muss mit System-GND verbunden sein — sonst floating
- **GPIO34/35** sind Input-Only, kein `pinMode OUTPUT` setzen
- **ADC2** (GPIO0/2/4/12-15/25-27) mit WiFi unbrauchbar → Sensor-AOUTs immer auf ADC1 (GPIO32-39)
- **Relais active-low**: LOW = Pumpe AN, HIGH = Pumpe AUS (in config.h definiert)
- **Boot-Reihenfolge**: `digitalWrite` vor `pinMode` in `soil_ctrl_init()` verhindert Boot-Glitch
