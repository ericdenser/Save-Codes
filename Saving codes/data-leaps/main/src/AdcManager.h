#ifndef ADCMANAGER_H
#define ADCMANAGER_H
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

class AdcManager {
public:
    static void init();
    static void configChannel(adc_channel_t channel); // Pin config
    static int readMilliVolts(adc_channel_t channel); // Read value
private:
    static adc_oneshot_unit_handle_t _adc_handle;
    static adc_cali_handle_t _adc_cali_handle;
    static bool _is_initialized;
};
#endif
