#include <avr/io.h>
#include "twi.h"


#define SEG7_PCF_ADDR 0x21


// Segment bit order (A to G): 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G
const uint8_t seg7_hex_map[15] = {
0b0000110, // 1
0b1011011, // 2
0b1001111, // 3
0b1100110, // 4
0b1101101, // 5
0b1111101, // 6
0b0000111, // 7
0b1111111, // 8
0b1101111, // 9
0b1110111, // A
0b1111100, // B
0b0111001, // C
0b1011110, // D
0b1111001, // E
0b1110001 // F
};


void seg7_init(void) {
TWI_Init(); // Make sure TWI is initialized once globally
}


void seg7_display_hex(uint8_t val) {
    if (val < 1) val = 1;
    else if (val > 15) val = 15;

    uint8_t segments = seg7_hex_map[val - 1]; // fix index!

    // PCF8574 is active LOW → invert bits
    uint8_t output = ~segments;

    TWI_MT_Start();
    TWI_Transmit_SLAW(SEG7_PCF_ADDR);
    TWI_Transmit_Byte(output);
    TWI_Stop();
}