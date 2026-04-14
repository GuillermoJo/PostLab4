#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

void ADC_init(void);
uint8_t ADC_read_8bit(void);

void mostrar_contador(uint8_t contador);

void desactivar_displays(void);
void activar_display1(void);
void activar_display2(void);
void enviar_segmentos(uint8_t patron);
uint8_t hex_a_7seg(uint8_t nibble);
void mostrar_hex_multiplexado(uint8_t valor);

int main(void)
{
	uint8_t contador = 0;
	uint8_t adc_valor = 0;

	ADC_init();

	// D2 y D3 como entradas para botones
	DDRD &= ~((1 << PD2) | (1 << PD3));

	// Pull-up interno en D2 y D3
	PORTD |= (1 << PD2) | (1 << PD3);

	// Salidas del contador:
	// D0, D4, D5, D6, D7
	DDRD |= (1 << PD0) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);

	// D8, D9, D10, D11, D12, D13
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) |
	(1 << PB3) | (1 << PB4) | (1 << PB5);

	// A0..A5 como salidas para segmentos b..g
	DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) |
	(1 << PC3) | (1 << PC4) | (1 << PC5);

	while (1)
	{
		// Leer ADC en A7
		adc_valor = ADC_read_8bit();

		// Mostrar contador binario en LEDs
		mostrar_contador(contador);

		// Refrescar displays multiplexados
		mostrar_hex_multiplexado(adc_valor);

		// Botón aumentar en D2
		if (!(PIND & (1 << PD2)))
		{
			_delay_ms(30);
			if (!(PIND & (1 << PD2)))
			{
				contador++;
				while (!(PIND & (1 << PD2)))
				{
					mostrar_hex_multiplexado(adc_valor);
					mostrar_contador(contador);
				}
				_delay_ms(30);
			}
		}

		// Botón disminuir en D3
		if (!(PIND & (1 << PD3)))
		{
			_delay_ms(30);
			if (!(PIND & (1 << PD3)))
			{
				contador--;
				while (!(PIND & (1 << PD3)))
				{
					mostrar_hex_multiplexado(adc_valor);
					mostrar_contador(contador);
				}
				_delay_ms(30);
			}
		}
	}

	return 0;
}

void ADC_init(void)
{
	// AVcc como referencia, ajuste a la izquierda, canal ADC7
	ADMUX = (1 << REFS0) | (1 << ADLAR) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);

	// Habilitar ADC, prescaler 128
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint8_t ADC_read_8bit(void)
{
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void mostrar_contador(uint8_t contador)
{
	// Limpiar salidas del contador en PORTD sin tocar D2 y D3
	PORTD &= ~((1 << PD0) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7));

	// Limpiar salidas del contador en PORTB sin tocar D11, D12 y D13
	PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2));

	// Bit 7 -> D0
	if (contador & (1 << 7)) PORTD |= (1 << PD0);

	// Bit 6 -> D4
	if (contador & (1 << 6)) PORTD |= (1 << PD4);

	// Bit 5 -> D5
	if (contador & (1 << 5)) PORTD |= (1 << PD5);

	// Bit 4 -> D6
	if (contador & (1 << 4)) PORTD |= (1 << PD6);

	// Bit 3 -> D7
	if (contador & (1 << 3)) PORTD |= (1 << PD7);

	// Bit 2 -> D8
	if (contador & (1 << 2)) PORTB |= (1 << PB0);

	// Bit 1 -> D9
	if (contador & (1 << 1)) PORTB |= (1 << PB1);

	// Bit 0 -> D10
	if (contador & (1 << 0)) PORTB |= (1 << PB2);
}

void mostrar_hex_multiplexado(uint8_t valor)
{
	uint8_t nibble_alto = (valor >> 4) & 0x0F;
	uint8_t nibble_bajo = valor & 0x0F;

	desactivar_displays();
	enviar_segmentos(hex_a_7seg(nibble_alto));
	activar_display1();
	_delay_ms(2);

	desactivar_displays();
	enviar_segmentos(hex_a_7seg(nibble_bajo));
	activar_display2();
	_delay_ms(2);
}

void desactivar_displays(void)
{
	PORTB &= ~((1 << PB3) | (1 << PB4));
}

void activar_display1(void)
{
	PORTB |= (1 << PB3);   // D11
	PORTB &= ~(1 << PB4);  // D12
}

void activar_display2(void)
{
	PORTB &= ~(1 << PB3);  // D11
	PORTB |= (1 << PB4);   // D12
}

void enviar_segmentos(uint8_t patron)
{
	// Segmento a -> D13 -> PB5
	if (patron & (1 << 0)) PORTB |= (1 << PB5);
	else PORTB &= ~(1 << PB5);

	// Segmentos b..g -> A0..A5
	if (patron & (1 << 1)) PORTC |= (1 << PC0); else PORTC &= ~(1 << PC0); // b
	if (patron & (1 << 2)) PORTC |= (1 << PC1); else PORTC &= ~(1 << PC1); // c
	if (patron & (1 << 3)) PORTC |= (1 << PC2); else PORTC &= ~(1 << PC2); // d
	if (patron & (1 << 4)) PORTC |= (1 << PC3); else PORTC &= ~(1 << PC3); // e
	if (patron & (1 << 5)) PORTC |= (1 << PC4); else PORTC &= ~(1 << PC4); // f
	if (patron & (1 << 6)) PORTC |= (1 << PC5); else PORTC &= ~(1 << PC5); // g
}

uint8_t hex_a_7seg(uint8_t nibble)
{
	switch (nibble)
	{
		case 0x0: return 0b00111111;
		case 0x1: return 0b00000110;
		case 0x2: return 0b01011011;
		case 0x3: return 0b01001111;
		case 0x4: return 0b01100110;
		case 0x5: return 0b01101101;
		case 0x6: return 0b01111101;
		case 0x7: return 0b00000111;
		case 0x8: return 0b01111111;
		case 0x9: return 0b01101111;
		case 0xA: return 0b01110111;
		case 0xB: return 0b01111100;
		case 0xC: return 0b00111001;
		case 0xD: return 0b01011110;
		case 0xE: return 0b01111001;
		case 0xF: return 0b01110001;
		default:  return 0b00000000;
	}
}

