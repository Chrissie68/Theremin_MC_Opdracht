#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <string.h>
#include <seg7_display.h>

#define MAX_FILTER  15 // Maximale filtergrootte
#define MIN_FILTER  1 // Minimale filtergrootte

volatile uint8_t filterSize = 1; // Huidige filtergrootte
volatile bool updateSeg7 = false;   

// Structure voor mediaan filter
typedef struct {
    uint8_t age;
    uint8_t value;
} GemetenAfstanden;

static GemetenAfstanden buffer[MAX_FILTER]; // Buffer voor mediaan filter

//PCINT ISR voor filtergrootte knoppen
ISR(PCINT2_vect)
{
    static uint8_t vorigeStatus = 0xFF;   // Vorige toestand van de poort
    uint8_t huidigeStatus = PIND;       // Huidige toestand

    // Knop op PD2 filter verkleinen
    if ((vorigeStatus & (1 << PD2)) && !(huidigeStatus & (1 << PD2))) {
        if (filterSize > MIN_FILTER) {
            filterSize--;
            updateSeg7 = true;
        }
    }
    // Knop op PD5 filter vergroten
    if ((vorigeStatus & (1 << PD5)) && !(huidigeStatus & (1 << PD5))) {
        if (filterSize < MAX_FILTER) {
            filterSize++;
            updateSeg7 = true;
        }
    }

    vorigeStatus = huidigeStatus; // Nieuwe toestand onthouden
}

void init_filterButtons(void)
{
    // Stel PD2 en PD5 in als input met interne pull-up
    DDRD &= ~((1 << DDD2) | (1 << DDD5));
    PORTD |= (1 << PORTD2) | (1 << PORTD5);

    // Pin Change Interrupts inschakelen voor poort D
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT21);

    // Startwaarde tonen op 7-segment display
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
    uint8_t maxAge = 0;

    // Oudste sample bepalen
    for (uint8_t i = 0; i < filterSize; i++) {
        buffer[i].age++;
        if (buffer[i].age > maxAge) {
            maxAge = buffer[i].age;
            oldest = i;
        }
    }

    // Oudste vervangen door nieuwe waarde
    buffer[oldest].value = afstand;
    buffer[oldest].age = 0;

    // Kopie maken en sorteren
    GemetenAfstanden temp[MAX_FILTER];
    memcpy(temp, buffer, sizeof(temp));
    qsort(temp, filterSize, sizeof(GemetenAfstanden), comp);

    // Mediaan teruggeven
    return temp[(filterSize / 2)].value;
}

uint8_t get_filter_size(void)
{
    return filterSize;
}

bool filter_size_changed(void)
{
    return updateSeg7;
}

void set_filter_size_changed(bool status)
{
    updateSeg7 = status;
}