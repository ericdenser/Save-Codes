#include "BleManager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"             
#include "esp_nimble_hci.h"     
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h" 
#include "freertos/event_groups.h"

static const char* TAG = "BleManager";

#define BLE_CONNECTED_BIT BIT0
#define BLE_SCANNING_BIT  BIT1

// ================= VARIAVEIS GLOBAIS =================
static BleConfig s_config = {"", false, BleMode::CLIENT};
static EventGroupHandle_t s_ble_event_group = NULL;
static DataReceivedCallback s_data_callback = nullptr;
static bool s_is_initialized = false;
static BleManager::BleStatus s_current_status = BleManager::BleStatus::IDLE;

// Variáveis de CLIENT 
static uint16_t s_client_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_client_attr_write_handle = 0;

// Variáveis de SERVER 
static uint16_t s_server_val_handle = 0;

static const ble_uuid16_t gatt_svc_uuid = BLE_UUID16_INIT(0xFFF0);
static const ble_uuid16_t gatt_chr_uuid = BLE_UUID16_INIT(0xFFF1);

// ================= PROTOTIPOS =================
static void ble_host_task(void* param);
static void ble_on_reset(int reason);
static void ble_on_sync(void);
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static void update_status(BleManager::BleStatus new_status);

// Métodos Client
static void start_scan();
static int ble_on_subscribe(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
static int ble_on_chr_disced(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg);
static int ble_on_svc_disced(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);
static int on_client_notify_rx(struct os_mbuf *om);

// Métodos Server
static void start_advertising();
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

// ================= DEFINIÇÃO DA TABELA GATT (SERVER) =================
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid.u,
        .includes = NULL,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .arg = NULL,
                .descriptors = NULL,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .min_key_size = 0,
                .val_handle = &s_server_val_handle,
            },
            {0,}
        },
    },
    {0,}
};

// ================= IMPLEMENTAÇÃO DOS METODOS DO BLEMANAGER =================

void BleManager::init(BleConfig config) {
    if(s_is_initialized) return;
    s_config = config;
    s_ble_event_group = xEventGroupCreate();

    //Init nvs
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE Port Init Failed: %d", ret);
        update_status(BleManager::BleStatus::FAILED);
        return;
    }

    // 3. Configura Callbacks do Host
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // 4. Configura Server (se necessário)
    if (s_config.mode == BleMode::SERVER) {
        int rc = ble_gatts_count_cfg(gatt_svr_svcs);
        if (rc != 0) {
            ESP_LOGE(TAG, "GATT count failed: %d", rc);
            return;
        }
        rc = ble_gatts_add_svcs(gatt_svr_svcs);
        if (rc != 0) {
            ESP_LOGE(TAG, "GATT add failed: %d", rc);
            return;
        }
    }

    // Inicia a Task do FreeRTOS
    nimble_port_freertos_init(ble_host_task);
    s_is_initialized = true;
    ESP_LOGI(TAG, "BLE Manager inicializado com sucesso!");
}

bool BleManager::waitForConnection(uint32_t timeout_ms) {
    if (!s_ble_event_group) return false;
    ESP_LOGI(TAG, "Aguardando conexão...");
    EventBits_t bits = xEventGroupWaitBits(s_ble_event_group, BLE_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return (bits & BLE_CONNECTED_BIT);
}

bool BleManager::isConnected() {
    return (s_current_status == BleStatus::CONNECTED);
}

BleManager::BleStatus BleManager::getStatus() {
    return s_current_status;
}

bool BleManager::sendData(const std::string& data) {
    if(!isConnected()) return false;
    
    if (s_config.mode == BleMode::CLIENT) {
        if (s_client_attr_write_handle == 0) return false;
        int rc = ble_gattc_write_no_rsp_flat(s_client_conn_handle, s_client_attr_write_handle, data.c_str(), data.length());
        return (rc == 0);
    } 
    else {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(data.c_str(), data.length());
        int rc = ble_gattc_notify_custom(s_client_conn_handle, s_server_val_handle, om);
        return (rc == 0);
    }
}

void BleManager::registerDataCallback(DataReceivedCallback cb) {
    s_data_callback = cb;
}

void BleManager::stop() {
    update_status(BleStatus::IDLE);
    // TODO: Adicionar lógica de parada do NimBLE se necessário
}

// ================= CALLBACKS INTERNOS =================

static void update_status(BleManager::BleStatus new_status) {
    if(s_current_status == new_status) return;
    s_current_status = new_status;

    if(s_ble_event_group) {
        if(new_status == BleManager::BleStatus::CONNECTED) {
            xEventGroupSetBits(s_ble_event_group, BLE_CONNECTED_BIT);
            xEventGroupClearBits(s_ble_event_group, BLE_SCANNING_BIT);
        } else if (new_status == BleManager::BleStatus::SCANNING) {
            xEventGroupSetBits(s_ble_event_group, BLE_SCANNING_BIT);
            xEventGroupClearBits(s_ble_event_group, BLE_CONNECTED_BIT);
        } else {
            xEventGroupClearBits(s_ble_event_group, BLE_CONNECTED_BIT | BLE_SCANNING_BIT);
        }
    }
}

static void ble_host_task(void* param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_on_reset(int reason) {
    ESP_LOGE(TAG, "BLE Reset: %d", reason);
    update_status(BleManager::BleStatus::IDLE);
}

static void ble_on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erro ao carregar endereço MAC");
        update_status(BleManager::BleStatus::FAILED);
        return;
    }

    if (s_config.mode == BleMode::CLIENT) {
        ESP_LOGI(TAG, "Modo CLIENT: Iniciando Scan...");
        start_scan();
    } else {
        ESP_LOGI(TAG, "Modo SERVER: Iniciando Advertising...");
        start_advertising();
    }
}

// ================= LÓGICA SERVER =================

static void start_advertising() {
    update_status(BleManager::BleStatus::ADVERTISING);
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    
    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)s_config.device_name;
    fields.name_len = strlen(s_config.device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        if (s_data_callback) {
            std::string data((char*)ctxt->om->om_data, ctxt->om->om_len);
            s_data_callback(data);
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// ================= LÓGICA CLIENT =================

static void start_scan() {
    update_status(BleManager::BleStatus::SCANNING);
    
    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params)); // Zera para evitar warnings

    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

static int on_client_notify_rx(struct os_mbuf *om) {
    if (s_data_callback && om != NULL) {
        std::string data((char*)om->om_data, om->om_len);
        s_data_callback(data);
    }
    return 0;
}

static int ble_on_subscribe(uint16_t conn_handle, const struct ble_gatt_error *error,
                            struct ble_gatt_attr *attr, void *arg) {
    if (error->status == 0) {
        ESP_LOGI(TAG, "Conexão Pronta!");
    } else {
        ESP_LOGE(TAG, "Erro ao subscrever: %d", error->status);
    }

    update_status(BleManager::BleStatus::CONNECTED);
    return 0;
}

static int ble_on_chr_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                             const struct ble_gatt_chr *chr, void *arg) {
    if (error->status != 0) return 0; 
    
    // Procura característica de escrita
    if (chr->properties & BLE_GATT_CHR_PROP_WRITE || chr->properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP) {
        ESP_LOGI(TAG, "Característica de Escrita: %d", chr->val_handle);
        s_client_attr_write_handle = chr->val_handle; 
    }
    
    // Procura característica de notificação
    if (chr->properties & BLE_GATT_CHR_PROP_NOTIFY) {
        ESP_LOGI(TAG, "Característica de Notificação: %d. Inscrevendo...", chr->val_handle);
        uint8_t val[2] = {1, 0}; 
        ble_gattc_write_flat(conn_handle, chr->val_handle + 1, val, sizeof(val), ble_on_subscribe, NULL);
    }
    return 0;
}

static int ble_on_svc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                             const struct ble_gatt_svc *service, void *arg) {
    if (error->status == 0) {
        ble_gattc_disc_all_chrs(conn_handle, service->start_handle, service->end_handle, ble_on_chr_disced, NULL);
    }
    return 0;
}

// ================= EVENT HANDLER =================

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Conectado (Handle: %d)", event->connect.conn_handle);
                s_client_conn_handle = event->connect.conn_handle;
                
                if (s_config.mode == BleMode::CLIENT) {
                    update_status(BleManager::BleStatus::DISCOVERING_SERVICES);
                    ble_gattc_disc_all_svcs(s_client_conn_handle, ble_on_svc_disced, NULL);
                } else {
                    update_status(BleManager::BleStatus::CONNECTED);
                }
            } else {
                if(s_config.mode == BleMode::CLIENT) start_scan();
                else start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(TAG, "Desconectado.");
            s_client_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            update_status(BleManager::BleStatus::DISCONNECTED);
            if (s_config.auto_reconnect) {
                if(s_config.mode == BleMode::CLIENT) start_scan();
                else start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISC:
            if (s_config.mode == BleMode::CLIENT) {
                struct ble_hs_adv_fields fields;
                if (ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data) == 0 && fields.name_len > 0) {
                    std::string name((char*)fields.name, fields.name_len);
                    if (name == s_config.device_name) {
                        ble_gap_disc_cancel();
                        update_status(BleManager::BleStatus::CONNECTING);
                        ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 30000, NULL, ble_gap_event, NULL);
                    }
                }
            }
            break;
            
        case BLE_GAP_EVENT_NOTIFY_RX:
             if (s_config.mode == BleMode::CLIENT) {
                 on_client_notify_rx(event->notify_rx.om);
             }
             break;
    }
    return 0;
}