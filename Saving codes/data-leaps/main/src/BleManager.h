#ifndef BLEMANAGER_H
#define BLEMANAGER_H

#include <string>
#include <functional>
#include <stdint.h>


enum class BleMode {
    CLIENT, 
    SERVER  
};

// Configuração básica
struct BleConfig {
    const char* device_name; 
    bool auto_reconnect = true;
    BleMode mode = BleMode::CLIENT;
};

// Callback para dados rápidos
typedef std::function<void(const std::string& data)> DataReceivedCallback;

class BleManager {
public:
    // Enum público para controle de estado na Main
    enum class BleStatus {
        IDLE,
        SCANNING, // Client only
        ADVERTISING, // Server only
        CONNECTING,
        DISCOVERING_SERVICES, 
        CONNECTED,         
        DISCONNECTED,
        FAILED
    };

    // Inicializa a stack Bluetooth
    static void init(BleConfig config);

    // Aguarda a conexão ser estabelecida (Bloqueante com timeout infinito)
    static bool waitForConnection(uint32_t timeout_ms);

    // Verifica se está conectado 
    static bool isConnected();

    // Retorna o estado atual 
    static BleStatus getStatus();

    //Se Client = Write, Se Server = Notify
    static bool sendData(const std::string& data);

    //Recebe tanto Notify (Client) quanto Write (Server)
    static void registerDataCallback(DataReceivedCallback cb);

    static void stop();
};

#endif