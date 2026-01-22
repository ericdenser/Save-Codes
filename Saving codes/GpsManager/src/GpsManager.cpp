#include "GpsManager.h" // Inclui o header correto
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
    #include "minmea.h"
}

static const char* TAG = "GPS_MANAGER"; // Tag atualizada

// MUDANÇA CRÍTICA: Troquei GpsDriver por GpsManager em tudo abaixo

GpsManager::GpsManager() {
    _lastCoordinates = "Aguardando satelites...";
    _lastDate = "Aguardando data...";
    _isInitialized = false;
    _uart_port = UART_NUM_1;
    float latitude = 0;
    float longitude = 0;
}

void GpsManager::init(int tx_pin, int rx_pin, uart_port_t port) {
    _uart_port = port;

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(_uart_port, GPS_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(_uart_port, &uart_config));
    uart_set_pin(_uart_port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    _isInitialized = true;
    ESP_LOGI(TAG, "GPS Inicializado na UART %d (TX: %d, RX: %d)", port, tx_pin, rx_pin);
}

std::string GpsManager::getCoordinates() {
    return _lastCoordinates;
}

int GpsManager::getLat() {
    return this->longitude;
}

int GpsManager::getLon() {
    return this->latitude;
}

std::string GpsManager::getDate() {
    return _lastDate;
}

void GpsManager::run() {
    if (!_isInitialized) {
        ESP_LOGE(TAG, "Erro: Execute init() antes de iniciar a task!");
        vTaskDelete(NULL);
        return;
    }

    uint8_t* data = (uint8_t*) malloc(GPS_BUF_SIZE);
    char line_buffer[MINMEA_MAX_SENTENCE_LENGTH];
    int line_pos = 0;


    char date_buffer[64];
    char coord_buffer[64];

    ESP_LOGI(TAG, "Task de leitura iniciada.");

    while (true) {
        int len = uart_read_bytes(_uart_port, data, GPS_BUF_SIZE, 100 / portTICK_PERIOD_MS);

        if (len > 0) {
            ESP_LOGI(TAG, "DADOS BRUTOS: %.*s", len, data);
            for (int i = 0; i < len; i++) {
                char c = (char)data[i];

                if (c == '\n' || c == '\r') {
                    line_buffer[line_pos] = '\0';
                    
                    if (minmea_sentence_id(line_buffer, false) == MINMEA_SENTENCE_RMC) {
                        struct minmea_sentence_rmc frame;
                        
                        if (minmea_parse_rmc(&frame, line_buffer)) {
                            if (frame.valid) {
                                float lat = minmea_tocoord(&frame.latitude);
                                float lon = minmea_tocoord(&frame.longitude);

                                int sec = frame.time.seconds;
                                int min = frame.time.minutes;
                                int hour = frame.time.hours;
                                int day = frame.date.day;
                                int month = frame.date.month;
                                int year = frame.date.year;

                                // Formata e salva a DATA/HORA
                                snprintf(date_buffer, sizeof(date_buffer), 
                                         "%02d/%02d/20%02d %02d:%02d:%02d",
                                         day, month, year, hour, min, sec);
                                _lastDate = std::string(date_buffer);

                                // Formata e salva a COORDENADA separadamente
                                snprintf(coord_buffer, sizeof(coord_buffer), 
                                         "Lat: %.8f, Lon: %.8f", lat, lon);
                                _lastCoordinates = std::string(coord_buffer);
                                latitude = lat;
                                longitude = lon;
                                
                            }
                        }
                    }
                    
                    line_pos = 0;
                } else {
                    if (line_pos < sizeof(line_buffer) - 1) {
                        line_buffer[line_pos++] = c;
                    }
                }
            }
        }
    }
    free(data);
}

void GpsManager::taskWrapper(void* _this) {
    static_cast<GpsManager*>(_this)->run();
}