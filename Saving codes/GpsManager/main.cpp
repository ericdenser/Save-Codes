
#include <iostream>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "GpsManager.h"
#include "HttpService.h"
#include "WifiManager.h"


#define PLACA_TX_PIN 17
#define PLACA_RX_PIN 18

// --- CONFIGURAÇÕES DO USUÁRIO ---
#define MY_WIFI_SSID      "PPGEEC"
#define MY_WIFI_PASS      "@2f0c2i3#"

#define MACK_URL       "https://mackleaps.mackenzie.br/datalogger/device"
#define MACK_API_KEY   "JYZ5g3zwgifqTeM4+DGzGg"

GpsManager gps;

static const char *TAG = "MAIN_APP";

extern "C" void app_main(void)
{
   // Inicializa NVS 
   esp_err_t ret = nvs_flash_init();
   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
   ESP_ERROR_CHECK(nvs_flash_erase());
   ret = nvs_flash_init();
   }
   ESP_ERROR_CHECK(ret);

   // Inicializar e Conectar Wi-Fi 
   WifiConfig conf;
   conf.ssid = MY_WIFI_SSID;
   conf.password = MY_WIFI_PASS;
   conf.max_retries = 5;

   WifiManager::init(conf);
   
   // Aguarda conexão 
   if (!WifiManager::waitForConnection([](){ })) {
      ESP_LOGE(TAG, "Falha fatal ao conectar no Wi-Fi.");
      WifiManager::recover();
   }


   // Inicializa GPS
   gps.init(PLACA_TX_PIN, PLACA_RX_PIN);

   xTaskCreate(GpsManager::taskWrapper, "gps_task", 4095, &gps, 5, NULL);

   ESP_LOGI(TAG, "Sistema Ativado, Aguardando dados...");


   std::string response;
   std::string msgOut;
   std::string dataHora;
   std::string localizacao;
   std::string macAddress = WifiManager::getMacAddress();


   while(1) {

      dataHora = gps.getDate();
      localizacao = gps.getCoordinates();
      float lat = gps.getLat();
      float lon = gps.getLon();

      // Imprime formatado no log
      ESP_LOGI(TAG, "Tempo: %s | Posição: %s", dataHora.c_str(), localizacao.c_str());

      // Verificação de Conexão 
      if(WifiManager::hasFailed()) {
         ESP_LOGE(TAG, "Critical. Not able to connect after 10 attempts.");
         WifiManager::recover();
      }

      // WiFi conectado
      if (WifiManager::getWifiStatus() == WifiManager::WifiStatus::WIFI_STATE_CONNECTED) {
         

         if(lat != 0.0 && lon != 0.0) {
            char json_payload[256];
            
            //"imprime" texto dentro da variavel json_payload
            snprintf(json_payload, sizeof(json_payload), 
            "{\"mac\": \"%s\", \"type\": \"gps\", \"lat\": %.6f, \"lon\": %.6f}", 
            macAddress.c_str(), lat, lon);

            ESP_LOGI(TAG, "Enviando POST para: %s com Payload: %s", MACK_URL, json_payload);


            if (HttpService::post(MACK_URL, json_payload, response, msgOut, "application/json", MACK_API_KEY)) {
               ESP_LOGI(TAG, "Sucesso POST! Resp: %s", response.c_str());
            } else {
               ESP_LOGE(TAG, "Falha POST: %s", msgOut.c_str());
            }
         } else {
                 ESP_LOGW(TAG, "GPS sem fix (0.0), não enviando para economizar dados.");
         }
         // Espera 5 segundos
         vTaskDelay(pdMS_TO_TICKS(3000));
      }
   }
}
