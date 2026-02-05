#include "DhtManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include <dht.h> 

static const char *TAG = "DhtManager";

// === CONFIGURAÇÕES ===
#define DHT_GPIO GPIO_NUM_5


#define SENSOR_TYPE DHT_TYPE_AM2301 

// Variáveis Estáticas
float DhtManager::_last_temp = 0.0;
float DhtManager::_last_hum = 0.0;

void DhtManager::init() {
    // Para sensores One-Wire, geralmente precisamos de um Pull-Up no pino de dados.
    // Muitos módulos já têm, mas ativar o interno do ESP garante que funcione.
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);
    
    // Pequeno delay para estabilizar a energia
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "DHT Configurado no GPIO %d", DHT_GPIO);
}

bool DhtManager::read() {
    float t, h;

    // dht_read_float_data(tipo_sensor, pino, &umidade, &temperatura)
    esp_err_t res = dht_read_float_data(SENSOR_TYPE, DHT_GPIO, &h, &t);

    if (res == ESP_OK) {
        _last_temp = t;
        _last_hum = h;
        ESP_LOGI(TAG, "Leitura OK: %.1f C, %.1f %%", t, h); // Descomente para debug
        return true;
    } else {
        ESP_LOGE(TAG, "Erro ao ler DHT: %d", res);
        return false;
    }
}

float DhtManager::getTemperature() { return _last_temp; }
float DhtManager::getHumidity() { return _last_hum; }