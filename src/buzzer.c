#include <avr/io.h>
#include <avr/interrupt.h>

#define BUZZER_PWM_DDR DDRD
#define BUZZER_PWM_PORT PORTD
#define BUZZER_PWM_PIN PD3 // OC2B = PD3

static uint8_t pwm_enabled = 1;

void init_buzzer(void)
{
    // Set PD3 (OC2B) as output
    BUZZER_PWM_DDR |= (1 << BUZZER_PWM_PIN);

    // --- Timer2: Fast PWM for volume control (62.5kHz)
    TCCR2A = (1 << WGM20) | (1 << WGM21);        // Fast PWM
    TCCR2A |= (1 << COM2B1);                     // Non-inverting mode on OC2B (PD3)
    TCCR2B = (1 << CS20);                        // No prescaling (fastest possible PWM)
    OCR2B = 0;                                   // Start with volume = 0
    TIMSK2 = 0;                                  // No Timer2 interrupts

    // --- Timer0: CTC mode to control the pitch
    TCCR0A = (1 << WGM01);                       // CTC mode
    TCCR0B = (1 << CS02);                        // Prescaler = 256
    TIMSK0 = (1 << OCIE0A);                      // Enable compare match interrupt
}

void set_buzzer_frequency(uint16_t freq)
{
    if (freq < 1) freq = 1;
    if (freq > 2000) freq = 2000;

    // Calculate OCR0A for CTC timer: f = F_CPU / (2 * prescaler * (1 + OCR0A))
    uint32_t ocr_val = (F_CPU / (2UL * 256 * freq)) - 1;

    if (ocr_val > 255)
        ocr_val = 255;

    OCR0A = (uint8_t)ocr_val;
}

void set_buzzer_volume(uint8_t volume)
{
    OCR2B = volume; // 0-255 duty cycle
}

// --- Timer0 CTC ISR: toggles PWM output by disabling/enabling OC2B
ISR(TIMER0_COMPA_vect)
{
    // Toggle OC2B output enable bit (COM2B1)
    if (pwm_enabled)
    {
        TCCR2A &= ~(1 << COM2B1); // Disable PWM output on PD3
        pwm_enabled = 0;
    }
    else
    {
        TCCR2A |= (1 << COM2B1);  // Enable PWM output on PD3
        pwm_enabled = 1;
    }
}
