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

#define LCD_ADDR    0x27 // I2C-adres van het LCD scherm
#define MAX_CM      60 // Maximale meetafstand in cm
#define MIN_CM      5 // Minimale meetafstand in cm
#define MAX_FILTER  15 // Maximale filtergrootte
#define MIN_FILTER  1 // Minimale filtergrootte
#define MIN_FREQ    230 // Minimale toonfrequentie in Hz
#define MAX_FREQ    1400 // Maximale toonfrequentie in Hz

volatile uint8_t filter_size = 1;   
volatile bool update_seg7 = false;    

// Structure voor mediaan filter
typedef struct {
    uint8_t age;
    uint8_t value;
} GemetenAfstanden;

static GemetenAfstanden buffer[MAX_FILTER]; // Buffer voor mediaan filter

//PCINT ISR voor filtergrootte knoppen
ISR(PCINT2_vect)
{
    static uint8_t prev_state = 0xFF;   // Vorige toestand van de poort
    uint8_t current_state = PIND;       // Huidige toestand

    // Knop op PD2 filter verkleinen
    if ((prev_state & (1 << PD2)) && !(current_state & (1 << PD2))) {
        if (filter_size > MIN_FILTER) {
            filter_size--;
            update_seg7 = true;
        }
    }

    // Knop op PD5 filter vergroten
    if ((prev_state & (1 << PD5)) && !(current_state & (1 << PD5))) {
        if (filter_size < MAX_FILTER) {
            filter_size++;
            update_seg7 = true;
        }
    }

    prev_state = current_state; // Nieuwe toestand onthouden
}

void init_filter_buttons(void)
{
    // Stel PD2 en PD5 in als input met interne pull-up
    DDRD &= ~((1 << DDD2) | (1 << DDD5));
    PORTD |= (1 << PORTD2) | (1 << PORTD5);

    // Pin Change Interrupts inschakelen voor poort D
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT21);

    // Startwaarde tonen op 7-segment display
    seg7_display_hex(filter_size);
}

// Vergelijkingsfunctie voor qsort
int comp(const void *a, const void *b)
{
    return (*(GemetenAfstanden *)a).value - (*(GemetenAfstanden *)b).value;
}

// Mediaan filter toepassen
uint8_t mediaan_filter(uint8_t afstand)
{
    uint8_t oldest = 0;
    uint8_t max_age = 0;

    // Oudste sample bepalen
    for (uint8_t i = 0; i < filter_size; i++) {
        buffer[i].age++;
        if (buffer[i].age > max_age) {
            max_age = buffer[i].age;
            oldest = i;
        }
    }

    // Oudste vervangen door nieuwe waarde
    buffer[oldest].value = afstand;
    buffer[oldest].age = 0;

    // Kopie maken en sorteren
    GemetenAfstanden temp[MAX_FILTER];
    memcpy(temp, buffer, sizeof(temp));
    qsort(temp, filter_size, sizeof(GemetenAfstanden), comp);

    // Mediaan teruggeven
    return temp[(filter_size / 2)].value;
}

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
    init_filter_buttons();
    HD44780_PCF8574_Init(LCD_ADDR);
    HD44780_PCF8574_DisplayOn(LCD_ADDR);

    sei();

    uint16_t trigger_timer = 0;
    uint16_t cm = 0;
    uint16_t filtered_cm = 0;
    uint16_t freq = 0;
    uint8_t volume = 0;

    while (1)
    {
        ultrasonic_tick();

        // Elke 60 ms nieuwe triggerpuls sturen(datasheet geeft 60 ms aan)
        if (++trigger_timer >= 60) {
            trigger_sensor();
            trigger_timer = 0;
        }

        // Update 7-segment display wanneer nodig
        if (update_seg7) {
            seg7_display_hex(filter_size);
            update_seg7 = false;
        }

        // Als een nieuwe afstand beschikbaar is
        if (ultrasonic_is_distance_ready()) {
            // Afstand uitlezen
            cm = ultrasonic_get_distance_cm();
            if(cm > MAX_CM)
            {
                cm = MAX_CM;
            } else if(cm < MIN_CM)
            {
                cm = MIN_CM;
            }
            // Mediaan filter toepassen
            filtered_cm = mediaan_filter(cm);
            // Frequentie berekenen op basis van gefilterde afstand(Formule staat in TO, hierbij word ook nog rekening gehouden met min en max afstand) 
            freq = MIN_FREQ + ((MAX_FREQ - MIN_FREQ) * (MAX_CM - filtered_cm)) / (MAX_CM - MIN_CM);

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
