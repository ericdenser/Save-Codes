#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"
#include "sensirion_config.h"

// Bibliotecas do ESP-IDF e do UncleRus
#include <i2cdev.h> 
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

// Variável que guarda a configuração I2C
static i2c_dev_t sps30_dev = { 0 };

/*
 * FUNÇÃO NOVA: Chama ela no seu C++ para configurar os pinos
 * Isso elimina a necessidade daquele "outro .h" que você viu no exemplo.
 */
void sps30_hal_init_pins(int sda, int scl) {
    sps30_dev.port = 0;                 // I2C Port 0
    sps30_dev.addr = 0x69;              // Endereço do SPS30
    sps30_dev.cfg.sda_io_num = (gpio_num_t)sda;
    sps30_dev.cfg.scl_io_num = (gpio_num_t)scl;
    sps30_dev.cfg.master.clk_speed = 100000; // 100kHz
    
    i2cdev_init(); // Garante que a lib está rodando
    i2c_dev_create_mutex(&sps30_dev);
}


// IMPLEMENTAÇÃO DAS FUNÇÕES QUE A SENSIRION PEDE 

void sensirion_i2c_hal_init(void) {
    // usamos a sps30_hal_init_pins 
}

void sensirion_i2c_hal_free(void) {
    i2c_dev_delete_mutex(&sps30_dev);
}

int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint8_t count) {
    sps30_dev.addr = address;
    esp_err_t err = i2c_dev_read(&sps30_dev, NULL, 0, data, (size_t)count);
    return (err == ESP_OK) ? 0 : -1;
}

int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data, uint8_t count) {
    sps30_dev.addr = address;
    // O cast (size_t) garante compatibilidade com a lib i2cdev
    esp_err_t err = i2c_dev_write(&sps30_dev, NULL, 0, data, (size_t)count);
    return (err == ESP_OK) ? 0 : -1;
}

void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    esp_rom_delay_us(useconds);
}