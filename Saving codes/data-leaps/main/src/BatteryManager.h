#ifndef BATTERYMANAGER_H
#define BATTERYMANAGER_H

#include <string>
#include "esp_http_client.h" 

class BatteryManager {
  public:
    // GET
    static float readBattery();
};
#endif