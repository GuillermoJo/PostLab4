#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define PONER_BITS(reg, mask)   ((reg) |=  (mask))
#define LIMPIAR_BITS(reg, mask) ((reg) &= ~(mask))
#define LEER_PIN(reg, pin)      ((reg) &   (1 << (pin)))

//Pines de botones
#define BTN_SUBIR     PD2
#define BTN_BAJAR     PD3
#define REBOTE_MS     25

//LED de alarma
#define LED_ALARMA    PD1

//Máscaras de salidas
#define PORTD_LEDS    ((1<<PD0)|(1<<PD4)|(1<<PD5)|(1<<PD6)|(1<<PD7))
#define PORTB_LEDS    ((1<<PB0)|(1<<PB1)|(1<<PB2))
#define DISP1_PIN     PB3
#define DISP2_PIN     PB4
#define SEG_A_PIN     PB5
#define SEGS_PORTB    ((1<<DISP1_PIN)|(1<<DISP2_PIN)|(1<<SEG_A_PIN))
#define SEGS_PORTC    ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5))

#define CICLOS_REFRESCO  20
#define RETARDO_MUX_MS   2

//Tabla de segmentos
static const uint8_t TABLA_7SEG[16] = {
    0x3F, 0x06, 0x5B, 0x4F,   /* 0-3 */
    0x66, 0x6D, 0x7D, 0x07,   /* 4-7 */
    0x7F, 0x6F, 0x77, 0x7C,   /* 8-B */
    0x39, 0x5E, 0x79, 0x71    /* C-F */
};

//Prototipos
static void    iniciar_adc(void);
static uint8_t leer_adc(void);
static void    salida_binaria(uint8_t valor);
static void    escribir_segmentos(uint8_t patron);
static void    elegir_display(uint8_t disp);
static void    mostrar_multiplexado(uint8_t valor);
static void    revisar_alarma(uint8_t adc, uint8_t contador);
static void    revisar_botones(uint8_t *contador, uint8_t *muestra);

//MAIN
int main(void)
{
    uint8_t contador = 0;
    uint8_t muestra  = 0;

    iniciar_adc();

    //Entradas con pull-up para botones
    LIMPIAR_BITS(DDRD,  (1<<BTN_SUBIR)|(1<<BTN_BAJAR));
    PONER_BITS(PORTD,   (1<<BTN_SUBIR)|(1<<BTN_BAJAR));

    //Salidas: LEDs contador + alarma
    PONER_BITS(DDRD, PORTD_LEDS | (1<<LED_ALARMA));

    //Salidas: displays y segmentos
    PONER_BITS(DDRB, PORTB_LEDS | SEGS_PORTB);
    PONER_BITS(DDRC, SEGS_PORTC);

    for (;;)
    {
        muestra = leer_adc();
        salida_binaria(contador);
        revisar_alarma(muestra, contador);

        //Refrescar displays y revisar botones en cada ciclo
        for (uint8_t i = 0; i < CICLOS_REFRESCO; i++)
        {
            mostrar_multiplexado(muestra);
            revisar_botones(&contador, &muestra);
        }
    }
}

//ADC
static void iniciar_adc(void)
{
    //AVcc como referencia, resultado alineado a la izquierda, canal ADC7
    ADMUX  = (1<<REFS0)|(1<<ADLAR)|(1<<MUX2)|(1<<MUX1)|(1<<MUX0);
    //Habilitar ADC, prescaler /128 ? 125 kHz @ 16 MHz
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

static uint8_t leer_adc(void)
{
    PONER_BITS(ADCSRA, (1<<ADSC));
    while (LEER_PIN(ADCSRA, ADSC));  //esperar fin de conversión
    return ADCH;                      //ADLAR=1 ? 8 bits en ADCH
}

//CONTADOR BINARIO EN LEDs
static void salida_binaria(uint8_t valor)
{
    //Limpiar solo pines del contador, sin tocar alarma ni displays
    LIMPIAR_BITS(PORTD, PORTD_LEDS);
    LIMPIAR_BITS(PORTB, PORTB_LEDS);

    //Bits 7-3 ? D0, D4-D7
    if (valor & (1<<7)) PONER_BITS(PORTD, (1<<PD0));
    if (valor & (1<<6)) PONER_BITS(PORTD, (1<<PD4));
    if (valor & (1<<5)) PONER_BITS(PORTD, (1<<PD5));
    if (valor & (1<<4)) PONER_BITS(PORTD, (1<<PD6));
    if (valor & (1<<3)) PONER_BITS(PORTD, (1<<PD7));

    //Bits 2-0 ? D8-D10
    if (valor & (1<<2)) PONER_BITS(PORTB, (1<<PB0));
    if (valor & (1<<1)) PONER_BITS(PORTB, (1<<PB1));
    if (valor & (1<<0)) PONER_BITS(PORTB, (1<<PB2));
}

//ALARMA
static void revisar_alarma(uint8_t adc, uint8_t contador)
{
    if (adc > contador)
        PONER_BITS(PORTD,   (1<<LED_ALARMA));
    else
        LIMPIAR_BITS(PORTD, (1<<LED_ALARMA));
}

//DISPLAY 7 SEGMENTOS MULTIPLEXADO
static void escribir_segmentos(uint8_t patron)
{
    //Segmento a ? PB5
    if (patron & (1<<0)) PONER_BITS(PORTB,   (1<<SEG_A_PIN));
    else                  LIMPIAR_BITS(PORTB, (1<<SEG_A_PIN));

    //Segmentos b-g ? PC0-PC5 en una sola operación
    PORTC = (PORTC & ~SEGS_PORTC) | ((patron >> 1) & 0x3F);
}

static void elegir_display(uint8_t disp)
{
    LIMPIAR_BITS(PORTB, (1<<DISP1_PIN)|(1<<DISP2_PIN));
    if (disp == 1) PONER_BITS(PORTB, (1<<DISP1_PIN));
    else           PONER_BITS(PORTB, (1<<DISP2_PIN));
}

static void mostrar_multiplexado(uint8_t valor)
{
    //Dígito alto
    LIMPIAR_BITS(PORTB, (1<<DISP1_PIN)|(1<<DISP2_PIN));
    escribir_segmentos(TABLA_7SEG[(valor >> 4) & 0x0F]);
    elegir_display(1);
    _delay_ms(RETARDO_MUX_MS);

    //Dígito bajo
    LIMPIAR_BITS(PORTB, (1<<DISP1_PIN)|(1<<DISP2_PIN));
    escribir_segmentos(TABLA_7SEG[valor & 0x0F]);
    elegir_display(2);
    _delay_ms(RETARDO_MUX_MS);
}

//BOTONES CON ANTIRREBOTE
static void revisar_botones(uint8_t *contador, uint8_t *muestra)
{
    //Botón subir
    if (!LEER_PIN(PIND, BTN_SUBIR))
    {
        _delay_ms(REBOTE_MS);
        if (!LEER_PIN(PIND, BTN_SUBIR))
        {
            (*contador)++;
            while (!LEER_PIN(PIND, BTN_SUBIR))
            {
                *muestra = leer_adc();
                salida_binaria(*contador);
                revisar_alarma(*muestra, *contador);
                mostrar_multiplexado(*muestra);
            }
        }
    }

    /* Botón bajar */
    if (!LEER_PIN(PIND, BTN_BAJAR))
    {
        _delay_ms(REBOTE_MS);
        if (!LEER_PIN(PIND, BTN_BAJAR))
        {
            (*contador)--;
            while (!LEER_PIN(PIND, BTN_BAJAR))
            {
                *muestra = leer_adc();
                salida_binaria(*contador);
                revisar_alarma(*muestra, *contador);
                mostrar_multiplexado(*muestra);
            }
        }
    }
}