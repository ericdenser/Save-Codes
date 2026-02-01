#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <string>
#include <functional>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"

class OtaManager {
  public:

    //Verifica se há atualizacão no servidor e inicia o download se necessário.
    static bool verify_and_update(int currentVersion, std::string &urlCheck, nvs_handle_t nvsHandle, std::string &msgOut,  std::function<void()> onProgressCallback = nullptr);

    //Marca o firmware atual como válido.
    static void set_valid_version();

    static void set_invalid_version(nvs_handle_t nvsHandler, int currentVersion);
  
  private:
    
    // Faz o download OTA do binário e aplica a atualizacão.
    static bool download_OTA(std::string &urlBin, nvs_handle_t nvsHandle, std::string &msgOut, std::function<void()> onProgressCallback = nullptr);
};

#endif