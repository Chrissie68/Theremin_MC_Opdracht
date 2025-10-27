#ifndef SEG7_DISPLAY_H
#define SEG7_DISPLAY_H

#include <stdint.h>

// I2C address of the 7-segment display's PCF8574 port expander
#define SEG7_I2C_ADDRESS 0x21

/**
 * @brief Initialize the 7-segment display (via I2C).
 *        This should be called once during setup.
 */
void seg7_init(void);

/**
 * @brief Display a hexadecimal digit (0–15 → 0–9, A–F) on the 7-segment display.
 *        If the value is out of range, the display will show blank.
 *
 * @param value A number between 0 and 15 (inclusive).
 */
void seg7_display_hex(uint8_t value);

#endif // SEG7_DISPLAY_H