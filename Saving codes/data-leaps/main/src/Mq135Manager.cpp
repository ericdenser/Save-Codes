#include "Mq135Manager.h"
#include "AdcManager.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "MQ135";

#define MQ135_ADC_CHANNEL  ADC_CHANNEL_5  // GPIO 6 no ESP32-S3

void Mq135Manager::init() {
    // config ADC
    AdcManager::configChannel(MQ135_ADC_CHANNEL);

    ESP_LOGI(TAG, "MQ-135 Inicializado (A: CH%d)", MQ135_ADC_CHANNEL);
}

float Mq135Manager::readVoltage() {
    int mv = AdcManager::readMilliVolts(MQ135_ADC_CHANNEL);
    return (float)mv / 1000.0f;
}
