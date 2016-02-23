/*
 * HM-Sensor-Test.c
 *
 * Created: 26.12.2015 16:11:49
 * Author : Martin
 */ 

#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <avr/wdt.h>
#include <util/delay.h>

#define LED_RED		PD4
#define LED_GREEN	PD3


int main(void)
{
	DDRD |= (_BV(LED_RED) | _BV(LED_GREEN));
	
    /* Replace with your application code */
    while (1) 
    {
		PORTD |= _BV(LED_GREEN);
		_delay_ms(500);
		PORTD &= ~_BV(LED_GREEN);
		_delay_ms(500);
		
		PORTD |= _BV(LED_RED);
		_delay_ms(500);
		PORTD &= ~_BV(LED_RED);
		_delay_ms(500);
    }
}

