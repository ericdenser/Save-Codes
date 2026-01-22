idf_component_register(
    SRCS 
        "main.cpp"              
        "src/GpsManager.cpp"    
        "src/minmea.c"
        "src/HttpService.cpp"
        "src/WifiManager.cpp"
    
    INCLUDE_DIRS 
        "." 
        "src"   
    REQUIRES
        "nvs_flash esp_wifi esp_event esp_http_client esp_timer"
                      
)