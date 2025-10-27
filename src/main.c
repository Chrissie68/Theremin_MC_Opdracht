#include <Arduino.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "ultrasonic.h"
#include "buzzer.h"
#include "hd44780pcf8574.h"

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define MAX_CM 60
#define MIN_CM 5

#define LCD_ADDR 0x27  // I2C address of your 2x16 LCD

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
    init_buzzer(); // Also initializes ADC
    sei();          // Enable global interrupts

    // Initialize LCD
    HD44780_PCF8574_Init(LCD_ADDR);
    HD44780_PCF8574_DisplayOn(LCD_ADDR);
    HD44780_PCF8574_DisplayClear(LCD_ADDR);

    const uint16_t fmin = 230;
    const uint16_t fmax = 1400;
    uint16_t trigger_timer = 0;

    while (1)
    {
        ultrasonic_tick();

        // Trigger ultrasonic sensor every ~50ms
        if (++trigger_timer >= 50)
        {
            trigger_sensor();
            trigger_timer = 0;
        }

        if (ultrasonic_is_distance_ready())
        {
            uint16_t cm = ultrasonic_get_distance_cm();
            if (cm > MAX_CM)       cm = MAX_CM;
            else if (cm < MIN_CM)  cm = MIN_CM;

            uint16_t freq = fmin + ((fmax - fmin) * (MAX_CM - cm)) / (MAX_CM - MIN_CM);
            set_buzzer_frequency(freq);

            uint8_t pot = get_pot_value();
            set_buzzer_volume(pot);

            // Update LCD display
            char line1[17];
            char line2[17];
            snprintf(line1, sizeof(line1), "Distance: %3ucm", cm);
            snprintf(line2, sizeof(line2), "Frequency:%4uHz", freq);

            HD44780_PCF8574_PositionXY(LCD_ADDR, 0, 0);
            HD44780_PCF8574_DrawString(LCD_ADDR, line1);
            HD44780_PCF8574_PositionXY(LCD_ADDR, 0, 1);
            HD44780_PCF8574_DrawString(LCD_ADDR, line2);
        }

        _delay_ms(1);
    }

    return 0;
}
