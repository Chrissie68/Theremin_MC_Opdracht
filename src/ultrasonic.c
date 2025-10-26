#include "ultrasonic.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define TRIG_PIN     PB1
#define ECHO_PIN     PB0

#define TRIG_PORT    PORTB
#define TRIG_DDR     DDRB
#define ECHO_DDR     DDRB

#define STATE_IDLE         0
#define STATE_START_PULSE  1
#define STATE_END_PULSE    2
#define STATE_WAIT         3

volatile uint8_t sensor_state = STATE_IDLE;
volatile uint16_t echo_start = 0;
volatile uint16_t echo_end = 0;
volatile bool distance_ready = false;
volatile uint16_t distance_cm = 0;

void init_ultrasonic(void)
{
    // Set TRIG as output, ECHO as input
    TRIG_DDR |= (1 << TRIG_PIN);   // PB1 as output
    ECHO_DDR &= ~(1 << ECHO_PIN);  // PB0 as input

    // Set trigger low
    TRIG_PORT &= ~(1 << TRIG_PIN);

    // Configure Timer1
    TCCR1A = 0x00;                             // Normal mode
    TCCR1B = (1 << ICES1) | (1 << CS11);       // Input Capture on Rising, Prescaler 8
    TIMSK1 |= (1 << ICIE1);                    // Enable Input Capture Interrupt
    sei();                                     // Enable global interrupts
}

void trigger_sensor(void)
{
    sensor_state = STATE_START_PULSE;
}

void ultrasonic_tick(void)
{
    static uint16_t start_time = 0;

    switch (sensor_state)
    {
        case STATE_START_PULSE:
            TRIG_PORT |= (1 << TRIG_PIN);     // Set trigger HIGH
            start_time = TCNT1;
            sensor_state = STATE_END_PULSE;
            break;

        case STATE_END_PULSE:
            if ((uint16_t)(TCNT1 - start_time) >= 160) // ~10 µs at 2 MHz
            {
                TRIG_PORT &= ~(1 << TRIG_PIN); // Set trigger LOW
                sensor_state = STATE_WAIT;
                distance_ready = false;
            }
            break;

        case STATE_WAIT:
            // Waiting for echo to complete (handled in ISR)
            break;

        default:
            break;
    }
}

bool ultrasonic_is_distance_ready(void)
{
    return distance_ready;
}

uint16_t ultrasonic_get_distance_cm(void)
{
    distance_ready = false;
    return distance_cm;
}

ISR(TIMER1_CAPT_vect)
{
    static bool waiting_for_falling = false;

    if (!waiting_for_falling)
    {
        echo_start = ICR1;
        TCCR1B &= ~(1 << ICES1); // switch to falling edge
        waiting_for_falling = true;
    }
    else
    {
        echo_end = ICR1;
        TCCR1B |= (1 << ICES1); // switch back to rising edge
        waiting_for_falling = false;

        uint16_t pulse_width = echo_end - echo_start;
        distance_cm = pulse_width / 58;
        distance_ready = true;
        sensor_state = STATE_IDLE;
    }
}
