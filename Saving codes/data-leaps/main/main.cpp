#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "esp_ota_ops.h"

#include "src/OtaManager.h"
#include "src/WifiManager.h"
#include "src/WatchdogManager.h" 
#include "src/BatteryManager.h" 
#include "src/BleManager.h"
#include "src/HttpService.h"

// OTA MACROS
#define FIRMWARE_VERSION 1
#define DEVICE_ID "ESP32S3_METEOROLOGIA_1"
#define TIME_TO_VALIDATE_OTA 30000000 // 30 seconds to consider firmware "stable"
#define MAX_CRASH_COUNT 3 // Maximum allowed crashes before forcing a rollback
#define URL_CHECK "http://192.168.15.52:8080/ciclo/firmware/check"

// BATTERY MACROS
#define BATTERY_MIN_VOLTAGE 3100.0

// DATA MACROS
#define MACK_URL          "https://mackleaps.mackenzie.br/datalogger/device"
#define MACK_API_KEY      "JYZ5g3zwgifqTeM4+DGzGg" 


// WIFI MACROS
#define DEFAULT_SSID ""
#define DEFAULT_PASS ""
#define MAX_WIFI_RETRIES 10

// Timeouts Health Check
#define WIFI_HEALTH_TIMEOUT_MS 10000

// Event group
static EventGroupHandle_t s_config_event_group;
#define CREDS_RECEIVED_BIT BIT0
#define CMD_EXIT_BIT       BIT1

// BLE CONFIG
#define CONFIG_TIMEOUT_MS 300000
#define BUTTON_GPIO GPIO_NUM_0

nvs_handle_t my_nvs_handle;
static const char *TAG = "MAIN_APP";
static std::string ota_msgOut;
static std::string currentProcess;
int current_voltage = 0.0f;
int8_t crashCount;
bool config_button_pressed = false;


RTC_DATA_ATTR int boot_count = 0;

// ======= MANUAL ROLL BACK HELPER =========
static void invalidate_version_and_rollback() {
    ESP_LOGE(TAG, "Unstable firmware detected! Initiating manual Rollback...");

    // Marks current partition as invalid and reboots to the previous one
    OtaManager::set_invalid_version(my_nvs_handle, FIRMWARE_VERSION);
}

// ======= HELPER NVS ==================
static void get_nvs_string(const char* key, char* out_buffer, size_t max_len) {
    size_t required_size;
    esp_err_t err = nvs_get_str(my_nvs_handle, key, NULL, &required_size);
    if (err == ESP_OK && required_size <= max_len) {
        nvs_get_str(my_nvs_handle, key, out_buffer, &required_size);
    } else {
        out_buffer[0] = '\0'; // Retorna vazio se der erro ou não existir
    }
}

// ======= BLE DATA CALLBACK =================
static void process_ble_data(const std::string& data) {
    ESP_LOGI("BLE_CB", "Dados: %s", data.c_str());

    cJSON *json = cJSON_Parse(data.c_str());
    if (json == NULL) {
        ESP_LOGE("BLE_CB", "JSON invalido");
        return;
    }

    // Check if it is a exit config command
    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    if (cJSON_IsString(cmd) && strcmp(cmd->valuestring, "exit") == 0) {
        ESP_LOGI("BLE_CB", "Leaving command received");
        if (s_config_event_group) xEventGroupSetBits(s_config_event_group, CMD_EXIT_BIT);
        cJSON_Delete(cmd);
        return;
    }

    // Process config
    nvs_handle_t handle;
    if (nvs_open("OtaManager", NVS_READWRITE, &handle) == ESP_OK) { 

        cJSON *ssid = cJSON_GetObjectItem(json, "wifi_ssid");
        cJSON *pass = cJSON_GetObjectItem(json, "wifi_pass");
        
        if (cJSON_IsString(ssid) && (ssid->valuestring != NULL)) {
            nvs_set_str(handle, "wifi_ssid", ssid->valuestring);
            ESP_LOGI("BLE_CB", "SSID saved: %s", ssid->valuestring);
        }
        if (cJSON_IsString(pass) && (pass->valuestring != NULL)) {
            nvs_set_str(handle, "wifi_pass", pass->valuestring);
            ESP_LOGI("BLE_CB", "Password saved");
        }

        // // Outras configs opcionais
        // cJSON *strategy = cJSON_GetObjectItem(json, "recovery_strategy");
        // if (cJSON_IsString(strategy)) {
        //      if(strcmp(strategy->valuestring, "RESET") == 0) nvs_set_u8(handle, "rec_strat", 0);
        //      else if(strcmp(strategy->valuestring, "SWAP") == 0) nvs_set_u8(handle, "rec_strat", 1);
        // }

        nvs_commit(handle);
        nvs_close(handle);
    }
    cJSON_Delete(json);

    ESP_LOGI("BLE_CB", "Configurações recebidas! Procedindo com Setup...");
    
    //Warns setup he can continue
    if (s_config_event_group != NULL) {
        xEventGroupSetBits(s_config_event_group, CREDS_RECEIVED_BIT);
    }
}

// ======== CONFIG MODE ===========
static void setup_configuration() {
    ESP_LOGI(TAG, ">>>> JOINING CONFIG MODE (5min TIMEOUT) <<<<<< ");

    currentProcess = "BLE";
    // start ble
    BleConfig ble;
    ble.device_name = "ESP32_CONFIG";
    ble.mode = BleMode::SERVER;
    BleManager::init(ble);
    BleManager::registerDataCallback(process_ble_data);


    // Waiting loop (5 min or exit command)
    int64_t start_time = esp_timer_get_time();
    int64_t timeout_us = (int64_t)CONFIG_TIMEOUT_MS * 1000;
    bool keepRunning = true;

    while (keepRunning) {

        if ((esp_timer_get_time() - start_time) > timeout_us) {
            ESP_LOGW(TAG, "COFIG MODE TIMEOUT");
            keepRunning = false;
        }

        EventBits_t bits = xEventGroupWaitBits(s_config_event_group, 
            CMD_EXIT_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(100));

        if (bits & CMD_EXIT_BIT) {
            ESP_LOGI(TAG, "Leaving config mode.");
            keepRunning = false;
        }


        WatchdogManager::reset();
    }

    BleManager::stop();
    ESP_LOGI(TAG, "Config finished. Returning to cycle...");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

// ======= SETUP COMMON AND CRUCIAL COMPONENTS =======
static void setup_crucial() {
    s_config_event_group = xEventGroupCreate();

    currentProcess = "NVS_INIT";
    // NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(nvs_open("OtaManager", NVS_READWRITE, &my_nvs_handle));

    // Crash Logic
    currentProcess = "CRASH_DETECTOR";
    err = nvs_get_i8(my_nvs_handle, "crash_count", &crashCount);
    if (err == ESP_ERR_NVS_NOT_FOUND) crashCount = 0;

    // Watchdog
    currentProcess = "WATCHDOG_INIT";
    WatchdogManager::init(30000, true);
    WatchdogManager::addToCurrentTask();

    // Reset Reason Logic
    currentProcess = "RESET_REASON_DETECTOR";
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_WDT || reason == ESP_RST_TASK_WDT || reason == ESP_RST_PANIC) {
        crashCount++;
        nvs_set_i8(my_nvs_handle, "crash_count", crashCount);
        nvs_commit(my_nvs_handle);
    } else {
        nvs_set_i8(my_nvs_handle, "crash_count", 0);
        nvs_commit(my_nvs_handle);
    }

    if(crashCount >= MAX_CRASH_COUNT) {
        nvs_set_i8(my_nvs_handle, "crash_count", 0);
        nvs_commit(my_nvs_handle);
        invalidate_version_and_rollback();
    }
}

static bool check_ota() {
    currentProcess = "OTA_CHECK";
    ESP_LOGI(TAG, "Checking for updates on OtaManager...");

    std::string url_check{URL_CHECK};

    bool result = OtaManager::verify_and_update(
        FIRMWARE_VERSION, 
        url_check, 
        my_nvs_handle, 
        ota_msgOut, 
        WatchdogManager::reset 
    );

    // if (!result) {
    //      Scenario where ota failed
    // }

    return result;
}

static void readSensors() {
    ESP_LOGI(TAG, "ENTRANDO EM SENSORES");
    currentProcess = "READ_SENSORS";
    //float voltage_mv = BatteryManager::readBattery();
    current_voltage = 3500; //voltage_mv

}

static void sendData() {
    currentProcess = "SEND_DATA";
    // essa funcao precisa receber os dados de alguma forma, global ou parametro
    if (WifiManager::isConnected()) {

        char json_payload[300];
        std::string response, msgOut;

        // Coleta dados
        int rssi = WifiManager::getRssi();
        std::string ip = WifiManager::getIp();
        std::string ssid = WifiManager::getSSID();
        int reason = (int) esp_reset_reason(); // Pega o motivo do reset atual


        snprintf(json_payload, sizeof(json_payload),
                "{\"device\":\"%s\",\"mac\":\"%s\",\"version\":%d,\"ssid\":\"%s\",\"ip\":\"%s\",\"last_reset_reason\":%d,\"crash_count\":%d,\"voltage\":%.2d,\"boot\":%d,\"rssi\":%d}",
                DEVICE_ID, 
                WifiManager::getMacAddress().c_str(), 
                FIRMWARE_VERSION,
                ssid.c_str(),
                ip.c_str(),
                reason,
                crashCount, 
                current_voltage, 
                boot_count,
                rssi);

        ESP_LOGI(TAG, "Payload: %s", json_payload);

        ESP_LOGI(TAG, "Enviando POST...");
        HttpService::post(MACK_URL, json_payload, response, msgOut, "application/json", MACK_API_KEY);
    }
}

static bool wait_for_condition(std::function<bool()> condition, uint32_t timeout_ms, const char* label) {
    int64_t start = esp_timer_get_time();
    int64_t timeout_us = (int64_t)timeout_ms * 1000;

    ESP_LOGI(TAG, "Validando: %s ...", label);

    while (!condition()) {
        if ((esp_timer_get_time() - start) > timeout_us) {
            ESP_LOGE(TAG, "FALHA: Timeout ao validar %s", label);
            return false;
        }
        
        WatchdogManager::reset(); // feed watchdog
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "Success: %s validated.", label);
    return true;
}

static bool checkSystemHealth(int64_t boot_time) {
    ESP_LOGI(TAG, "ENTRANDO CHECKING HEALTH");
    bool wifi_valid = false;
    bool battery_valid = true;
    bool valid_firmware = false;
    

    currentProcess = "CHECK_BATTERY";
    // ============ Validate Battery Life =============
    if (current_voltage < BATTERY_MIN_VOLTAGE && current_voltage > 500) {
        ESP_LOGE(TAG, "Critical voltage! Entering permanent Deep sleep.");
        WifiManager::deinit();
        esp_deep_sleep_start(); // no wakeup = sleep forever
    }
    
    currentProcess = "CHECK_WIFI";
    // ============ Validate Wifi =================
    if (WifiManager::isConnected()) {
        ESP_LOGI(TAG, "Wifi Validado");
        wifi_valid = true;
    } else {
        if (!wait_for_condition([](){ return WifiManager::isConnected();}, WIFI_HEALTH_TIMEOUT_MS, "WiFi Check")) {
        ESP_LOGW(TAG, "Wifi not connected, changing to micro sd saving.");
        // logica do micro sd
        }
    }
    

    // ============ Validate Ble ===================
    // BleManager::BleStatus status = BleManager::getStatus();
    // if (status == BleManager::BleStatus::FAILED) {
    //     ESP_LOGI(TAG, "Ble failed to init");
    // } else {
    //     ESP_LOGI(TAG, "BLE Status: OK (%d)", (int)status);
    // }

    currentProcess = "CHECK_FIRMWARE";
    // ============ Validate Firmware ===============
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Current firmware is not verified, waiting 30 seconds.");

            int64_t now = esp_timer_get_time();
            int64_t time_since_boot = now - boot_time;
            int64_t time_remaining = TIME_TO_VALIDATE_OTA - time_since_boot;
            // During this timer, if any crash happens, we save and treat it next boot 
            if (time_remaining > 0) {
                ESP_LOGI(TAG, "Waiting for firmware estabilization (%lld seconds restantes)...", time_remaining / 1000000.0);
                
                int64_t wait_start = esp_timer_get_time();
                while ((esp_timer_get_time() - wait_start) < time_remaining) {
                    WatchdogManager::reset();
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }

            // If it gets here, the firmware is stable
            OtaManager::set_valid_version();
            valid_firmware = true;

            // We got through the "danger zone", we can zero the crash counter and validate all components
            nvs_set_i8(my_nvs_handle, "crash_count", 0);
            nvs_commit(my_nvs_handle);
        } else {
            ESP_LOGW(TAG, "Firmware was already verified, skipped waiting loop.");
            valid_firmware = true;
        }
    }
    return (wifi_valid && valid_firmware);
} 

// ======================= NORMAL CYCLE =======================
static void standard_cycle(int64_t boot_time) {
    ESP_LOGI(TAG, "ENTRANDO STANDARD CYCLLE");
    // Check credentials saved in flash
    char ssid_buffer[33] = {0};
    char pass_buffer[64] = {0};
    get_nvs_string("wifi_ssid", ssid_buffer, sizeof(ssid_buffer));
    get_nvs_string("wifi_pass", pass_buffer, sizeof(pass_buffer));

    // No wifi found, force config mode
    // if (strlen(ssid_buffer) == 0) {
    //     ESP_LOGE(TAG, "Sem WiFi configurado! Forçando modo de configuração.");
    //     setup_configuration();
    //     return; // After leaving config mode, go to deep sleep and try again
    // }

    currentProcess = "WIFI";
    // Wifi Init
    WifiConfig cfg;
    cfg.ssid = "Alencar";
    cfg.password = "gol686837";
    cfg.max_retries = MAX_WIFI_RETRIES;
    WifiManager::init(cfg);
    
    if(WifiManager::waitForConnection(WatchdogManager::reset)) {
        if(WifiManager::hasFailed()) {
            WifiManager::recover();
        }
    }

    if (WifiManager::isConnected()) check_ota();

    readSensors();
    if (checkSystemHealth(boot_time)) {
        sendData();
    } else {
        ESP_LOGE(TAG, "Health check failed. Skipping data post.");
    }
}


extern "C" void app_main(void)
{   
    // Record boot time for validation firmware logic
    int64_t boot_time = esp_timer_get_time();

    boot_count++;

    setup_crucial();

    // Treat button logic
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    // if (cause == ESP_SLEEP_WAKEUP_EXT0) {
    //     ESP_LOGW(TAG, "Wakeup by External Button! Entering Config Mode...");
    //     config_button_pressed = true;
    // } else {
    //     ESP_LOGI(TAG, "Wakeup by Timer. Standard Cycle.");
    //     config_button_pressed = false;
    // }


    if (config_button_pressed) {
        setup_configuration();
    } else {
        standard_cycle(boot_time);
    }

    ESP_LOGI(TAG, "Finalized cycle. Sleeping...");
    vTaskDelay(pdMS_TO_TICKS(100)); 


    WifiManager::stop();
    // Sleep for 60 seconds
    esp_sleep_enable_timer_wakeup(60000000); 
    esp_sleep_enable_ext0_wakeup(BUTTON_GPIO, 0);
    esp_deep_sleep_start();
}


 // ============= TEST 1 (NATIVE ROLLBACK SIMULATION) ==========

        /* Guru Meditation Error (Null Pointer)
           - Expected: Device resets immediately.
           - Firmware IMAGE will still be as "pending" ->  Bootloader triggers rollback

           - Uncomment the conditional below to test
        */

        // if (esp_timer_get_time() - boot_time > 5000000) { 
        //     ESP_LOGE(TAG, "Simulating fatal crash...");
        //     int *ptr = NULL;
        //     *ptr = 42; 
        // }

        // ============= TEST 2 (MANUAL ROLLBACK SIMULATION) ============
        
        /* Watchdog Timeout (Infinite Loop)
           - Expected: 'Task watchdog got triggered' after 10s.
           - Increments crash_count in NVS. 
           - After 3 reboots, it triggers 'invalidate_version_and_rollback'. 

           - Uncomment the conditional below to test
        */

    //     OtaManager::set_valid_version(); // Validamos para desativar rollback nativo e deixar o rollback manual tratar
    //     if (esp_timer_get_time() - boot_time > 500000) { 
    //         ESP_LOGI(TAG, "JOINING INFINITE LOOP AND CRASHING...");
    //         while (true) {
    //              // wdt triggers here
    //         }
    //     }
