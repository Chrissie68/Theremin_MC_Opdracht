#include <Arduino.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "ultrasonic.h"
#include "buzzer.h"
#include "hd44780pcf8574.h"
#include "seg7_display.h"

#define F_CPU 16000000UL
#define MAX_CM 60
#define MIN_CM 5
#define LCD_ADDR 0x27
#define SEG7_ADDR 0x21

static volatile uint8_t filter_size = 5;  
static const uint8_t MIN_FILTER = 1;
static const uint8_t MAX_FILTER = 15;
volatile bool update_seg7 = false;

ISR(PCINT2_vect)
{
    static uint8_t prev_state = 0xFF;
    uint8_t current_state = PIND;

    // PD2 = Decrease filter
    if ((prev_state & (1 << PD2)) && !(current_state & (1 << PD2))) {
        if (filter_size > MIN_FILTER) {
            filter_size--;
            update_seg7 = true;
        }
    }

    // PD5 = Increase filter
    if ((prev_state & (1 << PD5)) && !(current_state & (1 << PD5))) {
        if (filter_size < MAX_FILTER) {
            filter_size++;
            update_seg7 = true;
        }
    }

    prev_state = current_state;
}

void init_filter_buttons()
{
    // PD2 en PD5 als input met pull-up aanzetten
    DDRD &= ~((1 << DDD2) | (1 << DDD5));
    PORTD |= (1 << PORTD2) | (1 << PORTD5);

    // interrupt aanzetten voor PORTD
    PCICR |= (1 << PCIE2);

    //pin change interrupts aanzetten voor PD2 (PCINT18) en PD5 (PCINT21)
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT21);

    //1 weergeven bij startup
    seg7_display_hex(filter_size);
}


// Median filter
#define MAX_FILTER 15
struct ValueEntry { uint8_t age, value; };
struct ValueEntry buffer[MAX_FILTER];

int compare_values(const void *a, const void *b)
{
    return ((struct ValueEntry *)a)->value - ((struct ValueEntry *)b)->value;
}

uint8_t apply_median_filter(uint8_t new_value)
{
    uint8_t oldest = 0, max_age = 0;
    for (uint8_t i = 0; i < MAX_FILTER; i++) {
        if (i < filter_size) buffer[i].age++;
        if (buffer[i].age > max_age) {
            max_age = buffer[i].age;
            oldest = i;
        }
    }

    buffer[oldest].value = new_value;
    buffer[oldest].age = 0;

    struct ValueEntry temp[MAX_FILTER];
    memcpy(temp, buffer, sizeof(temp));
    qsort(temp, filter_size, sizeof(struct ValueEntry), compare_values);

    return temp[filter_size / 2].value;
}

int main(void)
{
    init_ultrasonic();
    init_buzzer();          
    init_filter_buttons();
    seg7_init();
    HD44780_PCF8574_Init(LCD_ADDR);
    HD44780_PCF8574_DisplayOn(LCD_ADDR);
    sei();

    const uint16_t fmin = 230;
    const uint16_t fmax = 1400;
    uint16_t trigger_timer = 0;

    while (1)
    {
        ultrasonic_tick();

        if (++trigger_timer >= 50) {
            trigger_sensor();
            trigger_timer = 0;
        }

        if (update_seg7) {
        seg7_display_hex(filter_size);
        update_seg7 = false;
        }

        if (ultrasonic_is_distance_ready())
        {
            uint16_t cm = ultrasonic_get_distance_cm();
            if (cm > MAX_CM) cm = MAX_CM;
            else if (cm < MIN_CM) cm = MIN_CM;

            uint16_t freq = fmin + ((fmax - fmin) * (MAX_CM - cm)) / (MAX_CM - MIN_CM);
            set_buzzer_frequency(freq);

            uint8_t raw = get_pot_value();
            uint8_t filtered = apply_median_filter(raw);
            set_buzzer_volume(filtered);

            // LCD
            char line1[17], line2[17];
            snprintf(line1, sizeof(line1), "Distance: %3ucm", cm);
            snprintf(line2, sizeof(line2), "Frequency:%4uHz", freq);
            HD44780_PCF8574_PositionXY(LCD_ADDR, 0, 0);
            HD44780_PCF8574_DrawString(LCD_ADDR, line1);
            HD44780_PCF8574_PositionXY(LCD_ADDR, 0, 1);
            HD44780_PCF8574_DrawString(LCD_ADDR, line2);

            // 7-segment
            seg7_display_hex(filter_size);
        }

        _delay_ms(1);
    }

    return 0;
}

