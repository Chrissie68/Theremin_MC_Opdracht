#include <avr/io.h>
#include <avr/interrupt.h>
#include "buzzer.h"

#define F_CPU 16000000UL

// Connected to PD3
#define BUZZER_DDR  DDRD
#define BUZZER_PORT PORTD
#define BUZZER_PIN  PORTD3

static volatile uint8_t toggle = 0;

void init_buzzer(void)
{
    BUZZER_DDR |= (1 << BUZZER_PIN); // Set PD3 as output

    // Timer0 CTC mode
    TCCR0A = (1 << WGM01); // CTC mode
    TCCR0B = (1 << CS02);  // Prescaler 256
    OCR0A = 255;           // Placeholder, will be updated
    TIMSK0 = (1 << OCIE0A); // Enable Compare Match A interrupt
}

void set_buzzer_frequency(uint16_t freq)
{
    if (freq == 0) {
        TIMSK0 &= ~(1 << OCIE0A); // Disable interrupt
        BUZZER_PORT &= ~(1 << BUZZER_PIN); // Silence
        return;
    }

    uint32_t interrupt_freq = 2UL * freq; // Half-period
    uint32_t ocr_val = (F_CPU / (256UL * interrupt_freq)) - 1;

    if (ocr_val > 255) ocr_val = 255;
    OCR0A = (uint8_t)ocr_val;

    TIMSK0 |= (1 << OCIE0A); // Ensure interrupt is enabled
}

ISR(TIMER0_COMPA_vect)
{
    toggle = !toggle;
    if (toggle)
        BUZZER_PORT |= (1 << BUZZER_PIN);
    else
        BUZZER_PORT &= ~(1 << BUZZER_PIN);
}
