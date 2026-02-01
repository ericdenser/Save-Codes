#include "OtaManager.h"
#include "HttpService.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "cJSON.h"
#include "esp_ota_ops.h"
#include "esp_crt_bundle.h"

static const char *TAG = "OtaManager";


static bool jsonHelper(std::string &jsonPayload, int &newVersion, std::string &urlBinario, std::string &msgOut) {

    cJSON *json = cJSON_Parse(jsonPayload.c_str());
    if (!json) {
        msgOut = "Erro: JSON Invalido.";
        ESP_LOGW(TAG, "%s", msgOut.c_str());
        return false;
    }

    cJSON *versionItem = cJSON_GetObjectItem(json, "version");
    cJSON *urlItem = cJSON_GetObjectItem(json, "url");

    if (!cJSON_IsNumber(versionItem) || !cJSON_IsString(urlItem)) {
        msgOut = "Erro: Formato JSON Inesperado.";
        ESP_LOGW(TAG, "%s", msgOut.c_str());
        cJSON_Delete(json);
        return false;
    }

    newVersion = versionItem->valueint;
    urlBinario = urlItem->valuestring;

    cJSON_Delete(json); // libera memória
    return true;
}


bool OtaManager::verify_and_update(int currentVersion, std::string &urlCheck, nvs_handle_t nvsHandle, std::string &msgOut, std::function<void()> onProgressCallback) {
    std::string jsonPayload;
    ESP_LOGI(TAG, "Checking on url:%s", urlCheck.c_str());
    bool sucess = HttpService::get(urlCheck, jsonPayload, msgOut);

    if (!sucess) {
        ESP_LOGE(TAG, "%s", msgOut.c_str());
        return false;
    }

    if (jsonPayload.empty()) {
        msgOut = "Erro: Falha ao baixar JSON ou JSON vazio.";
        ESP_LOGE(TAG, "%s", msgOut.c_str());
        return false;
    }

    int newVersion;
    std::string urlBinario; 

    if (!jsonHelper(jsonPayload, newVersion, urlBinario, msgOut)) {
        return false;
    }


    // ============= VERIFICACÃO DE VERSÕES INVÁLIDAS ================

    // Verificar se houve rollback nativo e baixou uma versão corrompida
    const esp_partition_t *invalid_partition = esp_ota_get_last_invalid_partition();

    // Recupera qual foi a ultima versão que tentamos baixar
    int32_t target_ver = -1;
    nvs_get_i32(nvsHandle, "target_ver", &target_ver);

    // Lógica de julgamento
    if (invalid_partition != NULL && newVersion == target_ver) {
        ESP_LOGE(TAG, "ALERTA: Detectado Rollback!");
        ESP_LOGE(TAG, "A particao %s foi marcada como INVALIDA.", invalid_partition->label);
        ESP_LOGE(TAG, "A ultima versao %d causou o rollback. A mesma será banida.", newVersion);

        /* Como a ultima versão baixada (corrompida) é a mesma que ainda está 
        disponível para baixar, invalidamos ela pois já tentamos e falhou */
        nvs_set_i32(nvsHandle, "invalid_ver", newVersion);
        nvs_commit(nvsHandle);

        msgOut = "Versao " + std::to_string(newVersion) + " banida por instabilidade. Update cancelado";
        return false;
    }

    // Verificar versão inválida no NVS
    int32_t invalid_ver = -1;
    esp_err_t err = nvs_get_i32(nvsHandle, "invalid_ver", &invalid_ver);
    bool doesExist = (err == ESP_OK);

    ESP_LOGI(TAG, "Versão Nova: %d | Versão atual: %d | Versão inválida: %d", newVersion, currentVersion, (int)invalid_ver);

    if (doesExist && newVersion == invalid_ver) {
        msgOut = "Versao " + std::to_string(newVersion) + "marcada como inválida. Update abortado.";
        ESP_LOGI(TAG, "%s", msgOut.c_str());
        return false;
    }

    // ==========================================================


    // Se chegou aqui, versão disponível é válida, apenas verifica se é nova
    if (newVersion > currentVersion) {
        msgOut = "Atualizacao encontrada! Baixando v" + std::to_string(newVersion);

        // Registra qual versão esta sendo baixada para tratamento futuro
        nvs_set_i32(nvsHandle, "target_ver", newVersion);
        nvs_commit(nvsHandle);

        ESP_LOGI(TAG, "%s", msgOut.c_str());

        return download_OTA(urlBinario, nvsHandle, msgOut, onProgressCallback);

    } else {
        msgOut = "Firmware já está na versão mais recente.";
        ESP_LOGI(TAG, "%s", msgOut.c_str());
        return true;
    }
}

void OtaManager::set_valid_version() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        ESP_LOGI(TAG, "IMAGE = %d", ota_state);
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {

            ESP_LOGI(TAG, "Primeiro boot apos OTA. Validando firmware...");

            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                ESP_LOGI(TAG, "Firmware marcado como VALIDO. Rollback cancelado.");

            } else {
                ESP_LOGE(TAG, "Falha ao marcar firmware como valido!");
            }
        } else {
            ESP_LOGW(TAG, "Firmware ja estava validado ou nao requer validacao.");
        }
    }
}

void OtaManager::set_invalid_version(nvs_handle_t nvsHandler, int currentVersion) {
    ESP_LOGE(TAG, "FALHA CRITICA DETECTADA NA VERSAO %d. INICIANDO ROLLBACK...", currentVersion);

    nvs_set_i32(nvsHandler, "invalid_ver", currentVersion);
    nvs_commit(nvsHandler);
    ESP_LOGW(TAG, "Versao %d banida no NVS. Nao sera baixada novamente.", currentVersion);

    esp_ota_mark_app_invalid_rollback_and_reboot();
}

bool OtaManager::download_OTA(std::string &urlBin, nvs_handle_t nvsHandle, std::string &msgOut, std::function<void()> onProgressCallback) {

    // Config do cliente OTA
    esp_http_client_config_t http_config {};
    http_config.url = urlBin.c_str();
    http_config.timeout_ms = 10000;

    if (urlBin.find("https") == 0) {
        // Se for HTTPS: usa SSL e anexa certificados
        http_config.crt_bundle_attach = esp_crt_bundle_attach;
        http_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    } else {
        // Se for HTTP: força transporte TCP simples e remove certificados
        http_config.crt_bundle_attach = NULL;
        http_config.transport_type = HTTP_TRANSPORT_OVER_TCP; 
    }

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &http_config;

    // Inicia o OTA
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);


    if (err != ESP_OK) {
        msgOut = "Falha OTA Begin: " + std::string(esp_err_to_name(err));
        ESP_LOGI(TAG, "%s", msgOut.c_str()); 
        return false;
    }

    // Loop de Download
    while (1) {
        // Baixa um pedaço
        err = esp_https_ota_perform(https_ota_handle);

        // Se for diferente, ou acabou ou deu erro
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break; // Sai do loop
        }

        // CALLBACK DO WATCHDOG
        if (onProgressCallback) {
            onProgressCallback();
        }
    }
    // Verifica se terminou com sucesso
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        msgOut = "Error: Download incomplete";
        ESP_LOGE(TAG, "%s", msgOut.c_str());
        esp_https_ota_abort(https_ota_handle);
        return false;
    }

    // Finaliza
    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);

    if (ota_finish_err == ESP_OK) {

        // Zera o contador de crash antes de reiniciar
        nvs_set_i32(nvsHandle, "crash_count", 0);
        nvs_commit(nvsHandle);
        
        msgOut = "Upgrade successful. Rebooting...";
        ESP_LOGI(TAG, "%s", msgOut.c_str());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
        return true;

    } else {

        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            msgOut = "Image validation failed, image is corrupted";
            ESP_LOGE(TAG, "%s", msgOut.c_str());
            return false;
        }

        msgOut = "OTA upgrade failed" + std::string(esp_err_to_name(ota_finish_err));
        ESP_LOGE(TAG, "%s", msgOut.c_str());
        return false;
    }
    return false;
}