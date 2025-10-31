#ifndef mediaanFilter_H
#define mediaanFilter_H

#include <stdbool.h>
#include <stdint.h>

void init_filterButtons(void);
int comp(const void *a, const void *b);
uint8_t mediaan_filter(uint8_t afstand);
uint8_t get_filter_size(void);
bool update_segment_display(void);
void set_filter_size_changed(bool status);
#endif