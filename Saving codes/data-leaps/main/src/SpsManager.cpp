#include "SpsManager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sps30_i2c.h" 

static const char* TAG = "SpsManager";

// Variáveis Estáticas
float SpsManager::_last_pm1_0 = 0.0;
float SpsManager::_last_pm2_5 = 0.0;
float SpsManager::_last_pm4_0 = 0.0;
float SpsManager::_last_pm10_0 = 0.0;
float SpsManager::_last_particle_size = 0.0;
bool SpsManager::_is_initialized = false;

// Declaração da função HAL externa
extern "C" {
    void sps30_hal_init_pins(int sda, int scl);
}

void SpsManager::init() {
    if (_is_initialized) return;

    // 1. Configura Hardware
    sps30_hal_init_pins(GPIO_NUM_1, GPIO_NUM_2);

    // 2. Acorda o sensor
    sps30_wake_up(); 
    
    // 3. Lê Serial
    char serial_number[32]; 
    int16_t ret = sps30_read_serial_number((int8_t*)serial_number, 32); 

    if (ret != 0) {
        ESP_LOGE(TAG, "SPS30 não respondeu! (Erro %d)", ret);
    } else {
        ESP_LOGI(TAG, "SPS30 Detectado! Serial: %s", serial_number);
        
        // 4. Inicia Medição (CORRIGIDO PARA C++)
        // Fazemos um CAST explícito para converter o número 0x03 (Float) 
        // para o tipo Enum que a função espera.
        sps30_output_format format = (sps30_output_format)0x03;
        
        ret = sps30_start_measurement(format);
        
        if (ret < 0) {
            ESP_LOGE(TAG, "Erro start medição: %d", ret);
        } else {
            _is_initialized = true;
            ESP_LOGI(TAG, "SPS30 Iniciado com Sucesso!");
        }
    }
}

bool SpsManager::read() {
    if (!_is_initialized) {
        init();
        if (!_is_initialized) return false;
    }

    uint16_t data_ready = 0;
    int16_t ret = sps30_read_data_ready_flag(&data_ready);
    
    if (ret < 0) return false;

    if (data_ready) {
        float pm1_0, pm2_5, pm4_0, pm10_0;
        float nc0_5, nc1_0, nc2_5, nc4_0, nc10_0;
        float typical_size;

        ret = sps30_read_measurement_values_float(
            &pm1_0, &pm2_5, &pm4_0, &pm10_0,
            &nc0_5, &nc1_0, &nc2_5, &nc4_0, &nc10_0,
            &typical_size
        );

        if (ret < 0) {
            ESP_LOGE(TAG, "Erro leitura valores: %d", ret);
            return false;
        }

        _last_pm1_0 = pm1_0;
        _last_pm2_5 = pm2_5;
        _last_pm4_0 = pm4_0;
        _last_pm10_0 = pm10_0;
        _last_particle_size = typical_size;
        
        return true;
    }

    return false;
}

// Getters
float SpsManager::getPm10() { return _last_pm1_0; }
float SpsManager::getPm25() { return _last_pm2_5; }
float SpsManager::getPm40() { return _last_pm4_0; }
float SpsManager::getPm100() { return _last_pm10_0; }
float SpsManager::getTypicalParticleSize() { return _last_particle_size; }