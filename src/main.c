#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <HardwareSerial.h>


#define TRIG PB1   // Pin 9
#define ECHO PB0   // Pin 8

void initUltrasonic(void)
{
  // TRIG as output
  DDRB |= (1 << TRIG);

  // ECHO as input
  DDRB &= ~(1 << ECHO);
}

uint16_t readDistance()
{
  uint32_t duration = 0;

  // Send 10 µs trigger pulse
  PORTB &= ~(1 << TRIG);      // TRIG LOW
  _delay_us(2);
  PORTB |= (1 << TRIG);       // TRIG HIGH
  _delay_us(10);
  PORTB &= ~(1 << TRIG);      // TRIG LOW

  // Wait for ECHO to go HIGH
  while (!(PINB & (1 << ECHO)));

  // Measure how long ECHO stays HIGH
  while (PINB & (1 << ECHO))
  {
    duration++;
    _delay_us(1);  // crude timing, 1 µs steps
  }

  // Convert to cm: duration [µs] / 58
  return (uint16_t)(duration / 58);
}

int main()
{
  Serial.begin(9600);
  initUltrasonic();
  sei();

  while (1)
  {
    uint16_t distance = readDistance();
    Serial.print(distance);
    _delay_ms(500);
  }

  return 0;
}