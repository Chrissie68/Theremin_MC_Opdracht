#ifndef ADC_POTMETER_H
#define ADC_POTMETER_H

#include <stdint.h>

void init_adc(void);
uint8_t get_pot_value(void);

#endif