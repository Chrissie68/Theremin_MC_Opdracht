#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t pot_value = 0;

void init_adc(void)
{
    ADMUX = (1 << ADLAR) | (1 << REFS0); // Left adjust, AVCC ref, ADC0 (PC0)
    ADCSRA = (1 << ADEN)  | (1 << ADSC)  | (1 << ADATE) |
             (1 << ADIE)  | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable, Start, Auto Trigger, Interrupt, Prescaler 128
    ADCSRB = 0x00; // Free running mode
}

ISR(ADC_vect)
{
    pot_value = ADCH; // Only 8 MSB used due to ADLAR
}

uint8_t get_pot_value(void)
{
    return pot_value;
}