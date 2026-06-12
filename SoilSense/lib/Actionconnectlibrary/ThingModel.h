#ifndef CONNECTHUB_THINGMODEL_H
#define CONNECTHUB_THINGMODEL_H

#include <Arduino.h>
#include "../ble/BLEManager.h"

// Fragment used by Wlan::publishRegistration().
// Contains the top-level BLE registration info and the thing_model object.
//
// SoilSense capabilities — muss mit der publish_telemetry()-Payload im
// connecthub-Adapter uebereinstimmen:
//   moisture     NUMBER  readable  (%)
//   pump         ON_OFF  readable
//   tank_status  ON_OFF  readable
//   measure      IMPULSE writeable (loest On-Demand-Messung aus)
static String buildThingModelFragment() {
  String payload = R"TM(
"ble": {
    "service_uuid": ")TM" + getBleServiceUuid() + R"TM(",
    "tx_characteristic_uuid": ")TM" + getBleTxUuid() + R"TM(",
    "rx_characteristic_uuid": ")TM" + getBleRxUuid() + R"TM("
  },
"thing_model": {
    "metadata": {
      "label": "SoilSense",
      "category": "Plants"
    },
    "ble": {
      "service_uuid": ")TM" + getBleServiceUuid() + R"TM(",
      "tx_characteristic_uuid": ")TM" + getBleTxUuid() + R"TM(",
      "rx_characteristic_uuid": ")TM" + getBleRxUuid() + R"TM("
    },
    "capabilities": [
      {
        "id": "moisture",
        "type": "NUMBER",
        "label": "Feuchte",
        "direction": "readable",
        "min": 0,
        "max": 100,
        "unit": "%",
        "featured": true
      },
      {
        "id": "pump",
        "type": "ON_OFF",
        "label": "Pumpe",
        "direction": "readable",
        "featured": true
      },
      {
        "id": "tank_status",
        "type": "ON_OFF",
        "label": "Tank",
        "direction": "readable",
        "featured": true
      },
      {
        "id": "measure",
        "type": "IMPULSE",
        "label": "Messen",
        "direction": "writeable"
      }
    ],
    "state": {
      "moisture": 0,
      "pump": "OFF",
      "tank_status": "OFF"
    }
  }

)TM";
  return payload;
}

#endif // CONNECTHUB_THINGMODEL_H
