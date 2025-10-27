#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void init_buzzer(void);
void set_buzzer_frequency(uint16_t freq);
void set_buzzer_volume(uint8_t volume);

#endif
