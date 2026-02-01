#include "HttpService.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h" 

static const char *TAG = "HttpService";

// Event handler continua igual
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0) {
        std::string *buf = static_cast<std::string*>(evt->user_data);
        buf->append((char*)evt->data, evt->data_len);
    }
    return ESP_OK;
}

// GET
bool HttpService::get(const std::string &url, std::string &response_buffer, std::string &msgOut, const char* authorization) {
    // passando GET e payload vazio
    return perform_request(url, HTTP_METHOD_GET, "", response_buffer, msgOut, NULL, authorization);
}

// POST
bool HttpService::post(const std::string &url, const std::string &payload, std::string &response_buffer, std::string &msgOut, const char* content_type, const char* authorization) {
    // passando POST e o payload
    return perform_request(url, HTTP_METHOD_POST, payload, response_buffer, msgOut, content_type, authorization);
}

// LÓGICA CENTRAL
bool HttpService::perform_request(const std::string &url, esp_http_client_method_t method, const std::string &payload, std::string &response_buffer, std::string &msgOut, const char* content_type, const char* authorization) {
    
    response_buffer.clear();

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = http_event_handler;
    config.user_data = &response_buffer;
    config.timeout_ms = 10000;
    config.crt_bundle_attach = esp_crt_bundle_attach; 
    // config.disable_auto_redirect = true; // Opcional

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        msgOut = "Falha ao inicializar cliente HTTP";
        ESP_LOGE(TAG, "%s", msgOut.c_str());
        return false;
    }

    // Configura o Método
    esp_http_client_set_method(client, method);

    // Se for POST ou PUT..., configura o corpo e header
    if (method == HTTP_METHOD_POST) {
        esp_http_client_set_post_field(client, payload.c_str(), payload.length());
        
        if (content_type != NULL) {
            esp_http_client_set_header(client, "Content-Type", content_type);
        }

    }

    if (authorization != NULL) {
        esp_http_client_set_header(client, "Authorization", authorization);
    }

    // Executa
    esp_err_t err = esp_http_client_perform(client);
    bool success = false;

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        // 200 e 201 = OK para post
        if(status == 200 || status == 201) {
            success = true;
            msgOut = "Request Successful (Code: " + std::to_string(status) + ")";
            ESP_LOGI(TAG, "%s", msgOut.c_str());
        } else {
            ESP_LOGE(TAG, "Erro HTTP Code: %d", status);
            msgOut = "Erro HTTP Code: " +  std::to_string(status);
        }
    } else {
        ESP_LOGE(TAG, "Erro na requisição: %s", esp_err_to_name(err));
        msgOut = "Erro na requisição: " + std::string(esp_err_to_name(err));
        response_buffer.clear();
    }

    esp_http_client_cleanup(client);
    return success;
}