#include <Arduino.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "ultrasonic.h"
#include "buzzer.h"
#include "hd44780pcf8574.h"
#include "seg7_display.h"
#include "twi.h"
#include "mediaanFilter.h"

#define LCD_ADDR    0x27 // I2C-adres van het LCD scherm
#define MAX_CM      65 // Maximale meetafstand in cm
#define MAX_FILTER  15 // Maximale filtergrootte
#define MIN_FILTER  1 // Minimale filtergrootte
#define MIN_FREQ    230 // Minimale toonfrequentie in Hz
#define MAX_FREQ    1400 // Maximale toonfrequentie in Hz

// LCD display bijwerken
void update_lcd_display(uint16_t distance, uint16_t freq)
{
    char line1[17], line2[17];
    snprintf(line1, sizeof(line1), "Distance: %3ucm", distance);
    snprintf(line2, sizeof(line2), "Frequency:%4uHz", freq);

    HD44780_PCF8574_PositionXY(LCD_ADDR, 0, 0);
    HD44780_PCF8574_DrawString(LCD_ADDR, line1);
    HD44780_PCF8574_PositionXY(LCD_ADDR, 0, 1);
    HD44780_PCF8574_DrawString(LCD_ADDR, line2);
}

int main(void)
{
    // Initialisaties
    init_ultrasonic();
    init_buzzer();
    TWI_Init(); 
    init_filterButtons();
    HD44780_PCF8574_Init(LCD_ADDR);
    HD44780_PCF8574_DisplayOn(LCD_ADDR);

    sei();

    uint16_t triggertimer = 0;
    uint16_t cm = 0;
    uint16_t cmGefilterd = 0;
    uint16_t freq = 0;
    uint8_t volume = 0;

    while (1)
    {
        ultrasonic_tick();

        // Elke 60 ms nieuwe triggerpuls sturen(datasheet geeft 60 ms aan)
        if (++triggertimer >= 60) {
            trigger_sensor();
            triggertimer = 0;
        }

        // Update 7-segment display wanneer nodig
        if (filter_size_changed()) {
            seg7_display_hex(get_filter_size());
             set_filter_size_changed(false);
        }

        // Als een nieuwe afstand beschikbaar is
        if (ultrasonic_is_distance_ready()) {
            // Afstand uitlezen
            cm = ultrasonic_get_distance_cm();
            if(cm > MAX_CM)
            {
                cm = MAX_CM;
            } 
            // Mediaan filter toepassen
            cmGefilterd = mediaan_filter(cm);
            // Frequentie berekenen op basis van gefilterde afstand(Formule staat in TO) 
            freq = MAX_FREQ - ((MAX_FREQ - MIN_FREQ) * (MAX_CM - cmGefilterd)) / MAX_CM;

            // Volume uitlezen en instellen
            volume = get_pot_value();
            set_buzzer_volume(volume);

            // Toonfrequentie bijwerken
            set_buzzer_frequency(freq);

            // LCD bijwerken
            update_lcd_display(cm, freq);
        }
    }

    return 0;
}
