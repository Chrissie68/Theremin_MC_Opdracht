#include <avr/io.h>
#include <avr/interrupt.h>

static uint8_t pwmEnabled = 1;
static volatile uint8_t potWaarde = 0;

// Initialize buzzer timers and ADC for volume control
void init_buzzer(void)
{
    //Timer2 Fast PWM voor volume
    DDRD |= (1 << PD3); // PD3 = output
    TCCR2A = (1 << WGM20) | (1 << WGM21); // Fast PWM mode
    TCCR2A |= (1 << COM2B1); // Non-inverting PWM op OC2B
    TCCR2B = (1 << CS20); // geen prescaler
    OCR2B = 0; // volume = 0
    TIMSK2 = 0; // No Timer2 interrupts

    // Timer0 CTC voor toonfrequentie
    TCCR0A = (1 << WGM01); // CTC mode
    TCCR0B = (1 << CS02); // Prescaler = 256
    TIMSK0 = (1 << OCIE0A); // compare interrupt aan
    
    // ADC voor potmeter volume
    ADMUX = (1 << ADLAR) | (1 << REFS0); 
    
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    ADCSRB = 0x00;                                
}

// toonfrequentie instellen
void set_buzzer_frequency(uint16_t freq)
{
    if (freq < 1) freq = 1;
    if (freq > 2000) freq = 2000;

    // f = F_CPU / (2 * prescaler * (1 + OCR0A))
    uint32_t ocrWaarde = (F_CPU / (2UL * 256 * freq)) - 1;
    if (ocrWaarde > 255)
        ocrWaarde = 255;

    OCR0A = (uint8_t)ocrWaarde;
}

// volume instellen
void set_buzzer_volume(uint8_t volume)
{
    OCR2B = volume; // 0–255
}

uint8_t get_pot_value(void)
{
    return potWaarde;
}

ISR(ADC_vect)
{
    potWaarde = ADCH;
}

// --- Timer0 compare match ISR: toggle PWM on/off (tone on/off) ---
ISR(TIMER0_COMPA_vect)
{
    if (pwmEnabled)
    {
        TCCR2A &= ~(1 << COM2B1); // Disable output
        pwmEnabled = 0;
    }
    else
    {
        TCCR2A |= (1 << COM2B1);  // Enable output
        pwmEnabled = 1;
    }
}


