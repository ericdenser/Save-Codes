#ifndef HTTPSERVICE_H
#define HTTPSERVICE_H

#include <string>
#include "esp_http_client.h" 

class HttpService {
  public:
    // GET
    static bool get(const std::string &url, std::string &response_buffer, std::string &msgOut, const char* authorization = NULL);

    // POST (content_type vem com valor default para facilitar)
    static bool post(const std::string &url, const std::string &payload, std::string &response_buffer, std::string &msgOut, const char* content_type = "application/json", const char* authorization = NULL);

  private:
    // Método auxiliar privado (contém a lógica central)
    static bool perform_request(const std::string &url, esp_http_client_method_t method, const std::string &payload, std::string &response_buffer, std::string &msgOut, const char* content_type, const char* authorization);
};

#endif