#include <Arduino.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "ultrasonic.h"
#include "buzzer.h"
#include "adc.h"

#define F_CPU 8000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX 60
#define MIN 5

void uart_init(void)
{
    UBRR0H = (MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_print(const char *str)
{
    while (*str)
    {
        uart_transmit(*str++);
    }
}

int main(void)
{
    uart_init();
    init_ultrasonic();
    init_buzzer();
    init_adc();
    sei(); // Enable global interrupts

    const uint16_t fmin = 230;
    const uint16_t fmax = 1400;
    uint16_t trigger_timer = 0;

    while (1)
    {
        ultrasonic_tick();

        // Trigger sensor every ~50ms
        if (++trigger_timer >= 50)
        {
            trigger_sensor();
            trigger_timer = 0;
        }

        if (ultrasonic_is_distance_ready())
        {
            uint16_t cm = ultrasonic_get_distance_cm();
            if (cm > MAX)       cm = MAX;
            else if (cm < MIN)  cm = MIN;

            // Map distance to frequency
            uint16_t freq = fmin + ((fmax - fmin) * (MAX - cm)) / (MAX - MIN);
            set_buzzer_frequency(freq);

            // Set volume from potmeter (0–255)
            //uint8_t pot = get_pot_value();

            static uint8_t last_pot = 255;
uint8_t pot = get_pot_value();
if (pot != last_pot) {
    char buf[20];
    sprintf(buf, "POT: %u\r\n", pot);
    uart_print(buf);
    last_pot = pot;
}

            set_buzzer_volume(pot);

            // Debug print
            char buffer[50];
            sprintf(buffer, "Distance: %u cm  Frequency: %u Hz  Volume: %u\r\n", cm, freq, pot);
            uart_print(buffer);
        }

        _delay_ms(1);
    }

    return 0;
}

