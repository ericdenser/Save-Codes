#include <stdio.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_adc/adc_oneshot.h"


#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "src/WifiManager.h"
#include "src/HttpService.h"

static const char *TAG = "MAIN_APP";

// --- CONFIGURAÇÕES DO USUÁRIO ---
#define MY_WIFI_SSID      "PPGEEC"
#define MY_WIFI_PASS      "@2f0c2i3#"

#define MACK_URL       "https://mackleaps.mackenzie.br/datalogger/device"
#define MACK_API_KEY   "JYZ5g3zwgifqTeM4+DGzGg"


#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_3   // GPIO 4
#define ADC_ATTEN      ADC_ATTEN_DB_11 // permite medir até 3.1 V

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle = NULL; 
bool calibrated = false;             

// setup adc
void setup_adc() {
    ESP_LOGI(TAG, "Configurando ADC");

    // Configurando unidade
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // Configurando canal
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // Configurando calibracão
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
    
    if (ret == ESP_OK) {
        calibrated = true;
        ESP_LOGI(TAG, "Calibração ativada com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao iniciar calibração: %s", esp_err_to_name(ret));
    }
}

// FUNÇÃO DE LEITURA
float ler_bateria() {
    int raw_value = 0;
    // Média de 64 amostras 
    for(int i=0; i<64; i++){
        int temp;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &temp));
        raw_value += temp;
    }
    raw_value /= 64;

    if (calibrated) {
        int voltage_mv = 0;

        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw_value, &voltage_mv));

        return voltage_mv * 2; // Multiplica por 2 (2 resistores em série, Vout *= 0.5)
    } 
    return 0;

    // //tensão_no_pino = fração * tensão_máxima
    // float voltage_pin = (raw_value / 4095.0) * 3100.0;
    
    // return voltage_pin * 2.0; 
}

extern "C" void app_main(void) {
    // Inicializa NVS 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configurar ADC
    setup_adc();

    // Inicializar e Conectar Wi-Fi 
    WifiConfig conf;
    conf.ssid = MY_WIFI_SSID;
    conf.password = MY_WIFI_PASS;
    conf.max_retries = 5;

    WifiManager::init(conf);
    
    // Aguarda conexão 
    if (!WifiManager::waitForConnection([](){ })) {
        ESP_LOGE(TAG, "Falha fatal ao conectar no Wi-Fi. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }

    ESP_LOGI(TAG, "Conectado! IP: %s", WifiManager::getIp().c_str());

    // Preparação da Requisicão
    int64_t start_time = esp_timer_get_time();
    std::string response;
    std::string msgOut;

    ESP_LOGI(TAG, ">>> INICIANDO ENVIO DE DADOS <<<");

    while (1) {
        // Coleta dados
        float volts_mv = ler_bateria();
        int64_t uptime_sec = (esp_timer_get_time() - start_time) / 1000000;

        // Print Serial para Debug Local
        ESP_LOGI(TAG, "STATUS -> Bateria: %.2f mV | Uptime: %lld s | RSSI: %d", 
                 volts_mv, uptime_sec, WifiManager::getRssi());

        // Verificação de Conexão 
        if(WifiManager::hasFailed()) {
            ESP_LOGE(TAG, "Critical. Not able to connect after 10 attempts.");
            WifiManager::recover();
        }

        // WiFi conectado
        if (WifiManager::getWifiStatus() == WifiManager::WifiStatus::WIFI_STATE_CONNECTED) {

            char json_payload[128];
            
            //"imprime" texto dentro da variavel json_payload
            snprintf(json_payload, sizeof(json_payload), 
             "{\"voltage\": %.2f, \"uptime\": %lld}", 
             volts_mv, uptime_sec);

            ESP_LOGI(TAG, "Enviando POST para: %s com Payload: %s", MACK_URL, json_payload);


            if (HttpService::post(MACK_URL, json_payload, response, msgOut, "application/json", MACK_API_KEY)) {
                ESP_LOGI(TAG, "Sucesso POST! Resp: %s", response.c_str());
            } else {
                ESP_LOGE(TAG, "Falha POST: %s", msgOut.c_str());
            }

            // Espera 5 segundos
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    
    }
}