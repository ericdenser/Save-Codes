#include "Mq135Manager.h"
#include "AdcService.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "MQ135";

#define MQ135_ADC_CHANNEL  ADC_CHANNEL_5  // GPIO 6 no ESP32-S3
#define MQ135_DIGITAL_PIN  GPIO_NUM_13

void Mq135Manager::init() {
    // config ADC
    AdcService::configChannel(MQ135_ADC_CHANNEL);

    ESP_LOGI(TAG, "MQ-135 Inicializado (A: CH%d)", MQ135_ADC_CHANNEL);
}

float Mq135Manager::readVoltage() {
    int mv = AdcService::readMilliVolts(MQ135_ADC_CHANNEL);
    return (float)mv / 1000.0f;
}
