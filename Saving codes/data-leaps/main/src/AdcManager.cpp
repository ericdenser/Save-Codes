#include "AdcManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

static const char *TAG = "AdcManager";

// Definições estáticas
adc_oneshot_unit_handle_t AdcService::_adc_handle = NULL;
adc_cali_handle_t AdcService::_adc_cali_handle = NULL;
bool AdcService::_is_initialized = false;

void AdcService::init() {
    if (_is_initialized) return;

    ESP_LOGI(TAG, "Iniciando ADC Manager (Unit 1)");

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1, 
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &_adc_handle));

    // Calibração (Curve Fitting)
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten   = ADC_ATTEN_DB_12, // Suporta até ~3100mV
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &_adc_cali_handle) == ESP_OK) {
        ESP_LOGI(TAG, "Calibração ativada com sucesso");
    } else {
        ESP_LOGW(TAG, "Calibração falhou/não suportada, usando raw");
    }

    _is_initialized = true;
}

void AdcService::configChannel(adc_channel_t channel) {
    if (!_is_initialized) init();

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    adc_oneshot_config_channel(_adc_handle, channel, &config);
}

int AdcService::readMilliVolts(adc_channel_t channel) {
    if (!_is_initialized) init();

    int raw = 0;
    int sum = 0;

    // Median
    for(int i=0; i<16; i++){
        int val;
        adc_oneshot_read(_adc_handle, channel, &val);
        sum += val;
    }
    raw = sum / 16;

    int voltage = 0;
    if (_adc_cali_handle) {
        adc_cali_raw_to_voltage(_adc_cali_handle, raw, &voltage);
    } else {
        // Fallback 
        voltage = raw * 3100 / 4095;
    }
    return voltage;
}