#include <stdio.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

#include "BatteryManager.h"
#include "AdcManager.h"

static const char *TAG = "BatteryManager";



// ================= ADC =================
#define BAT_ADC_CHANNEL ADC_CHANNEL_3 // GPIO 4  


// Handles
adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle = NULL;
bool calibrated = false;
bool adc_initialized = false;

float BatteryManager::readBattery() {
    // Garante que o canal está configurado
    AdcManager::configChannel(BAT_ADC_CHANNEL);
    
    int voltage_pin_mv = AdcManager::readMilliVolts(BAT_ADC_CHANNEL);
    
    // Sua correção do divisor de tensão
    float correction_factor = 2.02f; 
    
    return (float)voltage_pin_mv * correction_factor;
}

