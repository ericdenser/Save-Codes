#ifndef DHTMANAGER_H
#define DHTMANAGER_H

class DhtManager {
public:
    // Configura o pino
    static void init();

    // Tenta ler o sensor. Retorna true se deu certo.
    static bool read();
    
    // Getters
    static float getTemperature();
    static float getHumidity();

private:
    static float _last_temp;
    static float _last_hum;
};

#endif