idf_component_register(
    SRCS 
        "sensirion_common.c"
        "sensirion_i2c.c"
        "sensirion_i2c_hal.c"
        "sps30_i2c.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        i2cdev     # Biblioteca do UncleRus
        driver     # Para GPIO
        esp_timer  # <--- ADICIONE ISSO (Correção do erro)
)