#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include <string>
#include "driver/uart.h"
#include "driver/gpio.h"

#define GPS_BUF_SIZE 1024

class GpsManager {
private:
    std::string _lastCoordinates;
    std::string _lastDate;
    uart_port_t _uart_port;
    bool _isInitialized;
    float latitude;
    float longitude;

public:
    // Construtor
    GpsManager();

    // Inicializa a UART com os pinos desejados
    void init(int tx_pin, int rx_pin, uart_port_t port = UART_NUM_1);

    // Retorna a string formatada
    std::string getCoordinates();

    int getLat();

    int getLon();

    //Retorna data formatada
    std::string getDate();

    // Loop principal (bloqueante, deve rodar na task)
    void run();

    // Método estático para o FreeRTOS chamar
    static void taskWrapper(void* _this);
};

#endif 