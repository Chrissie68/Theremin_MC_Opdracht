#include "ultrasonic.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

typedef enum {      
    START_PULSE, // Trigger hoog
    END_PULSE, // Trigge laag na 10µs
    WAIT // Wachten op echo via ISR
} UltrasonicState;

volatile UltrasonicState sensor_state = WAIT;
volatile uint16_t echo_start = 0; // Tijdstip rising edge
volatile uint16_t echo_end = 0; // Tijdstip falling edge
volatile bool distance_ready = false; // Flag voor voltooide meting
volatile uint16_t distance_cm = 0;// Berekende afstand in cm

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
    if (sensor_state == WAIT) {
        sensor_state = START_PULSE;
        distance_ready = false;
    }
}

// logica voor de ultrasonic sensor
void ultrasonic_tick(void)
{
    static uint16_t pulse_start_time = 0;
    // State-machine voor het aansturen van de ultrasonic sensor
    switch (sensor_state)
    {
        case START_PULSE:
            // Trigger hoog begin 10 µs puls
            PORTB |= (1 << PB1);
            pulse_start_time = TCNT1;
            sensor_state = END_PULSE;
            break;

        case END_PULSE:
            // Houd trigger 10 µs hoog (160 ticks bij 2 MHz)
            if ((uint16_t)(TCNT1 - pulse_start_time) >= 160)
            {
                PORTB &= ~(1 << PB1); // Trigger laag
                sensor_state = WAIT;
            }
            break;

        case WAIT:
        default:
            // Echo wordt gemeten via ISR (Input Capture)
            break;
    }
}

// return true als er een nieuwe afstand beschikbaar is
bool ultrasonic_is_distance_ready(void)
{
    return distance_ready;
}

// return de laatst gemeten afstand in cm
uint16_t ultrasonic_get_distance_cm(void)
{
    distance_ready = false;
    return distance_cm;
}

// verwerkt rising en falling edges volgens TO hoofdstuk 6.1 en figuur 8
ISR(TIMER1_CAPT_vect)
{
    static bool waiting_for_falling = false;

    if (!waiting_for_falling)
    {
        // Eerste interrupt: rising edge ontvangen → start echo
        echo_start = ICR1;

        // Capture nu omzetten naar falling edge
        TCCR1B &= ~(1 << ICES1);
        
        waiting_for_falling = true;
    }
    else
    {
        // Tweede interrupt: falling edge ontvangen → einde echo
        echo_end = ICR1;

        // Capture terugzetten op rising edge voor volgende meting
        TCCR1B |= (1 << ICES1);

        waiting_for_falling = false;

        // Pulsbreedte berekenen (tijd tussen rising en falling)
        uint16_t pulse_width = echo_end - echo_start;

        // Afstand berekenen (formule: afstand(cm) = tijd(µs) / 58)
        // 1 tick = 0.5 µs (2 MHz timer)
        distance_cm = (pulse_width / 2) / 58;

        // Resultaat beschikbaar maken voor main-loop
        distance_ready = true;

        // State-machine terug naar WAIT
        sensor_state = WAIT;
    }
}
