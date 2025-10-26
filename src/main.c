#include <Arduino.h>          // Needed for Serial
#include "ultrasonic.h"       // Your sensor header

int main(void)
{
    init();                   // Required to initialize Arduino core (timers, etc.)
    Serial.begin(9600);       // Enable Serial (Arduino handles UBRR setup)
    init_ultrasonic();        // Initialize trigger pin, timer1, interrupt

    uint32_t last_trigger_time = millis();

    while (1)
    {
        ultrasonic_tick();    // Run state machine (send trigger, wait, etc.)

        if (millis() - last_trigger_time >= 50)
        {
            trigger_sensor();
            last_trigger_time = millis();
        }

        if (ultrasonic_is_distance_ready())
        {
            uint16_t cm = ultrasonic_get_distance_cm();
            Serial.print("Distance: ");
            Serial.print(cm);
            Serial.println(" cm");
        }
    }

    return 0;
}
