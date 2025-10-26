#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>

void init_ultrasonic(void);
void ultrasonic_tick(void);
void trigger_sensor(void);
bool ultrasonic_is_distance_ready(void);
uint16_t ultrasonic_get_distance_cm(void);

#endif