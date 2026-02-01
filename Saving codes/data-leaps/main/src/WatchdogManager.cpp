#include "WatchdogManager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char* TAG = "WatchdogManager";

void WatchdogManager::init(int timeout, bool panic){

    esp_task_wdt_config_t wdt_config = {}; // Inicializa 
    wdt_config.timeout_ms = timeout;
    wdt_config.trigger_panic = panic;
    wdt_config.idle_core_mask = (1 << portNUM_PROCESSORS) - 1; // Monitora core 0 e 1

    // Tenta reconfigurar primeiro caso o bootloader já tenha iniciado o WDT
    esp_err_t err = esp_task_wdt_reconfigure(&wdt_config);
    
    // Se não estava iniciado, inicia agora
    if (err == ESP_ERR_NOT_FOUND) {
        err = esp_task_wdt_init(&wdt_config);
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Watchdog started: %d ms, Panic: %s, Cores: %d", 
                 timeout, panic ? "ON" : "OFF", portNUM_PROCESSORS);
    } else {
        ESP_LOGE(TAG, "Error at initializing Watchdog: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Watchdog started: %d ms, Panic: %s", timeout, panic ? "ON" : "OFF");
} 

void WatchdogManager::addToCurrentTask() {

    esp_err_t err = esp_task_wdt_add(NULL);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Current Task added to WDT monitoring.");
    } else {
        ESP_LOGW(TAG, "Failed at adding task (might be added already): %s", esp_err_to_name(err));
    }
}

void WatchdogManager::removeFromCurrentTask() {

    esp_err_t err = esp_task_wdt_delete(NULL);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Current task removida do monitoramento do WDT.");
    } else {
        ESP_LOGW(TAG, "Falha ao deletar monitoramento da task: %s", esp_err_to_name(err));
    }
}

void WatchdogManager::reset() {
    esp_task_wdt_reset();
}