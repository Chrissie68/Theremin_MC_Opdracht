#include <avr/io.h>
#include <avr/interrupt.h>

#define BUZZER_PWM_DDR   DDRD
#define BUZZER_PWM_PORT  PORTD
#define BUZZER_PWM_PIN   PD3 // OC2B = PD3

static uint8_t pwm_enabled = 1;
static volatile uint8_t pot_value = 0;

// --- Initialize buzzer timers and ADC for volume control ---
void init_buzzer(void)
{
    // --- Timer2: Fast PWM for volume ---
    BUZZER_PWM_DDR |= (1 << BUZZER_PWM_PIN);       // PD3 = output
    TCCR2A = (1 << WGM20) | (1 << WGM21);           // Fast PWM mode
    TCCR2A |= (1 << COM2B1);                        // Non-inverting PWM on OC2B
    TCCR2B = (1 << CS20);                           // No prescaler = fast PWM (~62.5kHz)
    OCR2B = 0;                                      // Initial volume = 0
    TIMSK2 = 0;                                     // No Timer2 interrupts

    // --- Timer0: CTC for frequency generation ---
    TCCR0A = (1 << WGM01);                          // CTC mode
    TCCR0B = (1 << CS02);                           // Prescaler = 256
    TIMSK0 = (1 << OCIE0A);                         // Enable compare interrupt

    // --- ADC: Potmeter on PC0 ---
    ADMUX = (1 << ADLAR) | (1 << REFS0);            // Left adjust, AVCC ref, ADC0 (PC0)
    ADCSRA = (1 << ADEN)  | (1 << ADSC)  |          // Enable ADC, start conversion
             (1 << ADATE) | (1 << ADIE)  |          // Auto trigger, interrupt on complete
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler = 128
    ADCSRB = 0x00;                                  // Free running mode
}

// --- Set buzzer pitch frequency using Timer0 ---
void set_buzzer_frequency(uint16_t freq)
{
    if (freq < 1) freq = 1;
    if (freq > 2000) freq = 2000;

    // f = F_CPU / (2 * prescaler * (1 + OCR0A))
    uint32_t ocr_val = (F_CPU / (2UL * 256 * freq)) - 1;
    if (ocr_val > 255)
        ocr_val = 255;

    OCR0A = (uint8_t)ocr_val;
}

// --- Set PWM duty cycle (volume) ---
void set_buzzer_volume(uint8_t volume)
{
    OCR2B = volume; // 0–255
}

// --- Get last read potmeter value (0–255) ---
uint8_t get_pot_value(void)
{
    return pot_value;
}

// --- ADC complete interrupt: store potmeter value (only 8 MSBs) ---
ISR(ADC_vect)
{
    pot_value = ADCH;
}

// --- Timer0 compare match ISR: toggle PWM on/off (tone on/off) ---
ISR(TIMER0_COMPA_vect)
{
    if (pwm_enabled)
    {
        TCCR2A &= ~(1 << COM2B1); // Disable output
        pwm_enabled = 0;
    }
    else
    {
        TCCR2A |= (1 << COM2B1);  // Enable output
        pwm_enabled = 1;
    }
}


