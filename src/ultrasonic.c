#include "ultrasonic.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

typedef enum {      
    START_PULSE, // Trigger hoog
    END_PULSE, // Trigge laag na 10µs
    WAIT // Wachten op echo via ISR
} UltrasonicState;

volatile UltrasonicState sensorStatus = WAIT;
volatile uint16_t echoStart = 0; // Tijdstip rising edge
volatile uint16_t echoEnd = 0; // Tijdstip falling edge
volatile bool distanceReady = false; // Flag voor voltooide meting
volatile uint16_t cm = 0;// Berekende afstand in cm

void init_ultrasonic(void)
{
    // PB1 (Trigger) als output, PB0 (Echo) als input
    DDRB |= (1 << PB1);
    DDRB &= ~(1 << PB0);

    // Trigger-pin standaard laag
    PORTB &= ~(1 << PB1);

    // TCCR1A: Normal mode (geen waveform output)
    TCCR1A = 0x00;

    // TCCR1B:
    // ICES1 = 1 capture op rising edge
    // CS11 = 1 prescaler = 8, 16 MHz / 8 = 2 MHz ticks

    TCCR1B = (1 << ICES1) | (1 << CS11);

    // TIMSK1:
    // ICIE1 = 1 Input Capture Interrupt aan
    TIMSK1 |= (1 << ICIE1);
}
// start de ultrasonic sensor meting
void trigger_sensor(void)
{
    if (sensorStatus == WAIT) {
        sensorStatus = START_PULSE;
        distanceReady = false;
    }
}

// logica voor de ultrasonic sensor
void ultrasonic_tick(void)
{
    static uint16_t pulseStartTijd = 0;
    // State-machine voor het aansturen van de ultrasonic sensor
    switch (sensorStatus)
    {
        case START_PULSE:
            // Trigger hoog begin 10 µs puls
            PORTB |= (1 << PB1);
            pulseStartTijd = TCNT1;
            sensorStatus = END_PULSE;
            break;

        case END_PULSE:
            // Houd trigger 10 µs hoog (160 ticks bij 2 MHz)
            if ((uint16_t)(TCNT1 - pulseStartTijd) >= 160)
            {
                PORTB &= ~(1 << PB1); // Trigger laag
                sensorStatus = WAIT;
            }
            break;

        case WAIT:
        default:
            // wachten
            // Echo wordt gemeten via ISR (Input Capture)
            break;
    }
}

// return true als er een nieuwe afstand beschikbaar is
bool ultrasonic_is_distance_ready(void)
{
    return distanceReady;
}

// return de laatst gemeten afstand in cm
uint16_t ultrasonic_get_distance_cm(void)
{
    distanceReady = false;
    return cm;
}

// verwerkt rising en falling edges volgens TO hoofdstuk 6.1 en figuur 8
ISR(TIMER1_CAPT_vect)
{
    static bool wachtenOpVallen = false;

    if (!wachtenOpVallen)
    {
        // Eerste interrupt: rising edge ontvangen → start echo
        echoStart = ICR1;

        // Capture nu omzetten naar falling edge
        TCCR1B &= ~(1 << ICES1);
        
        wachtenOpVallen = true;
    }
    else
    {
        // Tweede interrupt: falling edge ontvangen → einde echo
        echoEnd = ICR1;

        // Capture terugzetten op rising edge voor volgende meting
        TCCR1B |= (1 << ICES1);

        wachtenOpVallen = false;

        // Pulsbreedte berekenen (tijd tussen rising en falling)
        uint16_t pulse_width = echoEnd - echoStart;

        // Afstand berekenen (formule: afstand(cm) = tijd(µs) / 58)
        // 1 tick = 0.5 µs (2 MHz timer)
        cm = pulse_width / 58;

        // Resultaat beschikbaar maken voor main-loop
        distanceReady = true;

        // State-machine terug naar WAIT
        sensorStatus = WAIT;
    }
}
