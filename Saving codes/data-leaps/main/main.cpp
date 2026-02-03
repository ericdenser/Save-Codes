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
#define FIRMWARE_VERSION 3
#define DEVICE_ID "ESP32S3_TESTE_1"
#define TIME_TO_VALIDATE_OTA 30000000 // 30 seconds to consider firmware "stable"
#define MAX_CRASH_COUNT 3 // Maximum allowed crashes before forcing a rollback
#define URL_CHECK "http://172.16.38.146:8080/ciclo/firmware/check"


// --- Pinos do LED RGB ---
#define LED_PIN_R GPIO_NUM_16
#define LED_PIN_G GPIO_NUM_17
#define LED_PIN_B GPIO_NUM_18

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

// global variables
nvs_handle_t my_nvs_handle;
static const char *TAG = "MAIN_APP";
static std::string ota_msgOut;
static std::string currentProcess;
int current_voltage = 0.0f;
int8_t crashCount;
bool config_button_pressed = false;
bool micro_sd_strategy = false;
int64_t boot_time;
RTC_DATA_ATTR int boot_count = 0;

// declarations
static void standard_cycle();
static void recover_wifi();
static bool is_backup_active();
static void set_backup_active(bool active);
static void check_backup_fallback();
static void saveDataOffline();
static void sendData();
static void setup_leds();
static void set_led_color(int r, int g, int b);


static void setup_leds() {

    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    // Bitmask of gpios
    io_conf.pin_bit_mask = (1ULL<<LED_PIN_R) | (1ULL<<LED_PIN_G) | (1ULL<<LED_PIN_B);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    
    // Start turned off
    set_led_color(0, 0, 0);

}

static void set_led_color(int r, int g, int b) {
    // 1 turn on, 0 turn off
    gpio_set_level(LED_PIN_R, r);
    gpio_set_level(LED_PIN_G, g);
    gpio_set_level(LED_PIN_B, b);
}

// ======= MANUAL ROLLBACK HELPER =========
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

    // EX JSON ESPERADO:
    // {
    // "wifi_ssid": "SuaRedePrincipal",
    // "wifi_pass": "SuaSenhaPrincipal",
    // "wifi_backup_ssid": "RedeDoVizinho",
    // "wifi_backup_pass": "SenhaDoVizinho"
    // }

    // {
    // "cmd": "exit"
    // }
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
        cJSON *backup_ssid = cJSON_GetObjectItem(json, "wifi_backup_ssid");
        cJSON *backup_pass = cJSON_GetObjectItem(json, "wifi_backup_pass");
        
        if (cJSON_IsString(ssid) && (ssid->valuestring != NULL)) {
            nvs_set_str(handle, "wifi_ssid", ssid->valuestring);
            ESP_LOGI("MAIN-BLE", "SSID saved: %s", ssid->valuestring);
        }
        if (cJSON_IsString(pass) && (pass->valuestring != NULL)) {
            nvs_set_str(handle, "wifi_pass", pass->valuestring);
            ESP_LOGI("MAIN-BLE", "Password saved");
        }
        if (cJSON_IsString(backup_ssid) && (backup_ssid->valuestring != NULL)) {
            nvs_set_str(handle, "wifi_backup_ssid", backup_ssid->valuestring);
            ESP_LOGI("MAIN-BLE", "BACKUP SSID saved: %s", backup_ssid->valuestring);
        }
        if (cJSON_IsString(backup_pass) && (backup_pass->valuestring != NULL)) {
            nvs_set_str(handle, "wifi_backup_pass", backup_pass->valuestring);
            ESP_LOGI("MAIN-BLE", "BACKUP PASS saved");
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

    ESP_LOGI("BLE_CB", "Configurações recebidas!");
    
    //Warns setup he can continue
    if (s_config_event_group != NULL) {
        xEventGroupSetBits(s_config_event_group, CREDS_RECEIVED_BIT);
    }
}

// ======== CONFIG MODE ===========
static void setup_configuration() {
    ESP_LOGI(TAG, ">>>> JOINING CONFIG MODE (5min TIMEOUT) <<<<<< ");
    // Liga led azul (BLE LIGADO)
    set_led_color(0, 0, 1);
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
    // Turn off led
    set_led_color(0, 0, 0);
    standard_cycle();
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

    // Setup leds
    setup_leds();

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
    float voltage_mv = BatteryManager::readBattery();
    current_voltage = voltage_mv; 

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
        bool success = HttpService::post(MACK_URL, json_payload, response, msgOut, "application/json", MACK_API_KEY);
        if (success) {
            ESP_LOGI(TAG, "POST ENVIADO COM SUCESSO!");
            return;
        }
        ESP_LOGI(TAG, "FALHA AO ENVIAR POST");
        // Led amarelo (falha no post)
        set_led_color(1, 1, 0);
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

static bool checkSystemHealth() {
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

                // Turn purple to indicate Validation Timer
                set_led_color(1, 0, 1);
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
    set_led_color(0, 0, 0);
    return (wifi_valid && valid_firmware);
} 

// ======================= NORMAL CYCLE =======================
static void standard_cycle() {
    ESP_LOGI(TAG, "ENTRANDO STANDARD CYCLE");

    check_backup_fallback();

    bool use_backup = is_backup_active();
    const char* ssid_key = use_backup ? "wifi_backup_ssid" : "wifi_ssid";
    const char* pass_key = use_backup ? "wifi_backup_pass" : "wifi_pass";

    // Check credentials saved in flash
    char ssid[33] = {0};
    char pass[64] = {0};
    get_nvs_string(ssid_key, ssid, sizeof(ssid));
    get_nvs_string(pass_key, pass, sizeof(pass));
    

    // No wifi found, force config mode
    if (strlen(ssid) == 0) {
        ESP_LOGE(TAG, "No WiFi configured! Forcing config mode.");
        setup_configuration();

        // Check credentials again (should be new)
        get_nvs_string("wifi_ssid", ssid, sizeof(ssid));
        get_nvs_string("wifi_pass", pass, sizeof(pass));

        // If still empty, abort
        if (strlen(ssid) == 0) {
             ESP_LOGW(TAG, "Config canceled or invalid. Sleeping...");
             return;
        }
        ESP_LOGI(TAG, "New credentials found, trying connection...");
    }

    currentProcess = "WIFI";
    // Wifi Init
    WifiConfig cfg;
    cfg.ssid = ssid;
    cfg.password = pass;
    cfg.max_retries = MAX_WIFI_RETRIES;
    WifiManager::init(cfg);
    
    bool connected = WifiManager::waitForConnection(WatchdogManager::reset);

    // Failed after all attempts
    if (!connected) {
        recover_wifi(); 
        // Check if the backup ssid connected
        if (WifiManager::isConnected()) connected = true;
    } else {
        // If connected, restart the failure counter 
        int8_t streak = 0;
        nvs_get_i8(my_nvs_handle, "fail_streak", &streak);
        if (streak > 0) {
            nvs_set_i8(my_nvs_handle, "fail_streak", 0);
            nvs_commit(my_nvs_handle);
            ESP_LOGI(TAG, "Stable connection. Failure counter zeroed.");
        }

    }

    if (WifiManager::isConnected()) check_ota();

    readSensors();

    bool system_health = checkSystemHealth(); 

    // POST or SD?
    if (WifiManager::isConnected() && system_health) {
        set_led_color(0, 1, 0);
        sendData(); // send HTTP
    } else {
        ESP_LOGW(TAG, "HEALTH OR CONNECTION FAILED, ENTERING MICRO SD LOGIC.");

        // Led turn red, health check failed
        set_led_color(1, 1, 0);
        saveDataOffline(); // save in MicroSD
    }
}

static void recover_wifi() {
    // 1 = reset
    // 2 = backup ssid
    // 3 = tenta conexao todo ciclo
    int8_t fail_streak = 0;
    nvs_get_i8(my_nvs_handle, "fail_streak", &fail_streak);
    
    fail_streak++; 
    nvs_set_i8(my_nvs_handle, "fail_streak", fail_streak);
    nvs_commit(my_nvs_handle);

    switch(fail_streak) {
        // First Strategy (try a simple reset)
        case 1: 
            ESP_LOGI(TAG, "Trying simple reset to reload wifi driver");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
            break;
        // Second Strategy (try backup ssid)
        case 2: {
            bool current_is_backup = is_backup_active();

            const char* target_ssid_key = current_is_backup ? "wifi_ssid" : "wifi_backup_ssid";
            const char* target_pass_key = current_is_backup ? "wifi_pass" : "wifi_backup_pass";
            
            ESP_LOGW(TAG, "STRATEGY 2: Switching to ssid %s", current_is_backup ? "PRINCIPAL" : "BACKUP");

            char ssid[33] = {0};
            char pass[64] = {0};
            get_nvs_string(target_ssid_key, ssid, sizeof(ssid));
            get_nvs_string(target_pass_key, pass, sizeof(pass));
            
            if (strlen(ssid) > 0) {

            WifiManager::deinit();

                WifiConfig newCfg;
                WifiConfig cfg;
                cfg.ssid = ssid;
                cfg.password = pass;
                cfg.max_retries = MAX_WIFI_RETRIES;
                WifiManager::init(cfg);
                

                if (WifiManager::waitForConnection(WatchdogManager::reset)) {
                    ESP_LOGI(TAG, "Success: connected on the backup wifi!");

                    set_backup_active(!current_is_backup);

                    // Returns to the first recover strategy
                    nvs_set_i8(my_nvs_handle, "fail_streak", 0);
                    nvs_commit(my_nvs_handle);

                    return;
                } else {
                    ESP_LOGE(TAG, "FAIL: Backup Wifi failed too.");
                }
            } else {
                ESP_LOGW(TAG, "No backup found. Skipping strategy.");
            }
            break;
        }
        // Third strategy: Nothing esp can do, will keep trying connection with the first ssid
        case 3 ... 10:
            ESP_LOGW(TAG, "All strategies failed, returning to standart cycle.");
            break;
        // Until we stablish connection, all next failures will end here, which means it will ignore and keep trying connection/micro sd saving.
        default:
            ESP_LOGW(TAG, "Too many fails, restarting strategy cycle.");
            nvs_set_i8(my_nvs_handle, "next_strategy", 0);
            nvs_commit(my_nvs_handle);
            break;
    }

}

static bool is_backup_active() {
    int8_t val = 0;
    nvs_get_i8(my_nvs_handle, "use_backup", &val);
    return (val == 1);
}

static void set_backup_active(bool active) {
    nvs_set_i8(my_nvs_handle, "use_backup", active ? 1 : 0);
    nvs_commit(my_nvs_handle);
}

// Helper to count boots on backup mode
static void check_backup_fallback() {
    if (!is_backup_active()) {
        // If we are on the main ssid, zero counter
        nvs_set_i32(my_nvs_handle, "backup_boots", 0);
        nvs_commit(my_nvs_handle);
        return;
    }

    int32_t boots = 0;
    nvs_get_i32(my_nvs_handle, "backup_boots", &boots);
    boots++;

    ESP_LOGW(TAG, "Using backup ssid. Cycle: %ld/100", boots);

    if (boots >= 100) {
        ESP_LOGW(TAG, "Limit backup boots reached. Trying to go back to main ssid...");
        
        set_backup_active(false); 

        nvs_set_i32(my_nvs_handle, "backup_boots", 0);

    } else {
        // Still below the limit, just increment
        nvs_set_i32(my_nvs_handle, "backup_boots", boots);
    }
    nvs_commit(my_nvs_handle);
}

static void saveDataOffline() {
    currentProcess = "SAVE_OFFLINE";
    ESP_LOGW(TAG, ">> MODO OFFLINE: Salvando dados no MicroSD (Simulação) <<");
    // TODO: Implementar lógica real do SD aqui
    // mount_sd();
    // append_file("/sd/log.txt", json...);
    // unmount_sd();
}


extern "C" void app_main(void)
{   
    // Record boot time for validation firmware logic
    boot_time = esp_timer_get_time();

    boot_count++;

    setup_crucial();

    // Treat button logic
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGW(TAG, "Wakeup by External Button! Entering Config Mode...");
        config_button_pressed = true;
    } else {
        ESP_LOGI(TAG, "Wakeup by Timer. Standard Cycle.");
        config_button_pressed = false;
    }


    if (config_button_pressed) {
        setup_configuration();
    } else {
        standard_cycle();
    }

    ESP_LOGI(TAG, "Finalized cycle. Sleeping...");
    vTaskDelay(pdMS_TO_TICKS(2000)); 


    WifiManager::stop();
    set_led_color(0, 0, 0);

    // Sleep for 60 seconds
    esp_sleep_enable_timer_wakeup(60000000); 

    rtc_gpio_pullup_en(BUTTON_GPIO);
    rtc_gpio_pulldown_dis(BUTTON_GPIO);

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

