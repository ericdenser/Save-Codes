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

#include "src/BatteryManager.h"



static const char *TAG = "BatteryManager";

#define BATTERY_MIN_VOLTAGE 3100.0   // mV

// ================= ADC =================
#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_3      // GPIO 4 
#define ADC_ATTEN      ADC_ATTEN_DB_12     


// Handles
adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle = NULL;
bool calibrated = false;
bool adc_initialized = false;

// ================= ADC SETUP =================
static void setup_adc()
{
    ESP_LOGI(TAG, "Inicializando ADC");

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .chan    = ADC_CHANNEL,
        .atten   = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);

    if (ret == ESP_OK) {
        calibrated = true;
        adc_initialized = true;
        ESP_LOGI(TAG, "ADC calibrado (curve fitting)");
    } else {
        adc_initialized = true;
        ESP_LOGW(TAG, "Calibração ADC não suportada: %s", esp_err_to_name(ret));
    }
}

// ================= LEITURA DE BATERIA =================
float BatteryManager::readBattery()
{   
    if (!adc_initialized) {
        setup_adc();
    }
    
    int raw = 0;
    // Média de 64 leituras para estabilizar
    for (int i = 0; i < 64; i++) {
        int sample;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &sample));
        raw += sample;
    }
    raw /= 64;

    int voltage_pin_mv = 0;

    if (calibrated) {
        // Converte RAW (0-4095) para Milivolts reais no pino (ex: 1880 mV)
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_pin_mv));
    } else {
        // Fallback aproximado se calibração falhar (S3: 4095 ~= 3100mV com DB_12)
        // Isso é impreciso, use apenas se o 'if calibrated' falhar
        voltage_pin_mv = raw * 3100 / 4095; 
    }

    // Fator do Divisor de Tensão
    // Se estiver lendo errado, ajuste este número.
    // Ex: No multímetro estava dando 3,72, e no pino 3,68, (3720 / 3684) * 2.0 = 2.0195... -> 2.02
    
    float correction_factor = 2.02f; 

    return (float)voltage_pin_mv * correction_factor;
}

