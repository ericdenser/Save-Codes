#ifndef SPSMANAGER_H
#define SPSMANAGER_H

class SpsManager {
public:
    // Inicializa I2C e configura o sensor
    static void init();

    // Lê os dados do sensor e armazena internamente
    // Retorna true se a leitura foi bem sucedida
    static bool read();

    // Getters dos valores de Material Particulado (PM)
    // PM 1.0 (Partículas ultra finas)
    static float getPm10(); 
    
    // PM 2.5 (Padrão de qualidade do ar)
    static float getPm25(); 
    
    // PM 4.0
    static float getPm40();
    
    // PM 10.0 (Poeira grossa)
    static float getPm100();

    // Tamanho médio típico das partículas (em micrômetros)
    static float getTypicalParticleSize();

private:
    // Mantemos os valores estáticos para acesso global
    static float _last_pm1_0;
    static float _last_pm2_5;
    static float _last_pm4_0;
    static float _last_pm10_0;
    static float _last_particle_size;
    static bool _is_initialized;
};

#endif