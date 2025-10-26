#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "ultrasonic.h"

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

void uart_init(void)
{
    UBRR0H = (MYUBRR >> 8);
    UBRR0L = MYUBRR;
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

    uint16_t trigger_timer = 0;

    while (1)
    {
        ultrasonic_tick(); // handles trigger state machine

        // Trigger every ~50ms
        if (++trigger_timer >= 50)
        {
            trigger_sensor();
            trigger_timer = 0;
        }

        // If measurement is done, print immediately
        if (ultrasonic_is_distance_ready())
        {
            uint16_t cm = ultrasonic_get_distance_cm();

            char buffer[30];
            sprintf(buffer, "Distance: %u cm\r\n", cm);
            uart_print(buffer);
        }

        _delay_ms(1); // 1ms tick
    }

    return 0;
}


