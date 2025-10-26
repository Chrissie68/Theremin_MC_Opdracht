#include <avr/io.h>
#include <avr/interrupt.h>

#define BUZZER_PIN PD3

void init_buzzer(void)
{
    DDRD |= (1 << BUZZER_PIN); // PD3 as output

    TCCR0A = (1 << WGM01);     // CTC mode
    TCCR0B = (1 << CS02);      // Prescaler = 256
    TIMSK0 = (1 << OCIE0A);    // Enable Timer0 Compare Match A interrupt
}

void set_buzzer_frequency(uint16_t freq)
{
    if (freq < 1) freq = 1;
    if (freq > 2000) freq = 2000; // safety clamp

    // f = F_CPU / (2 * N * (1 + OCR0A))  ==> solve for OCR0A
    uint32_t ocr_val = (F_CPU / (2UL * 256 * freq)) - 1;

    if (ocr_val > 255)
        ocr_val = 255;
    
    OCR0A = (uint8_t)ocr_val;
}

// Square wave output on PD3
ISR(TIMER0_COMPA_vect)
{
    PORTD ^= (1 << BUZZER_PIN); // Toggle buzzer pin
}

