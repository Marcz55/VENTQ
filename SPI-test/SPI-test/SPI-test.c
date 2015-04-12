
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_CPU 16000000UL
#include <util/delay.h>

unsigned char inbuffer = 0b10101010;
unsigned char trash;

void spi_init(void)
{
	DDRA = (1<<PORTA2)|(1<<PORTA0);
	PORTA = (1<<PORTA0);
	PORTA = (1<<PORTA2); //Ha ingen slav vald
	DDRB = (1<<PORTB5)|(1<<PORTB7)|(1<<PORTB4); //Definiera outputs
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0); //Sätt enhet till master, enable spi, klockfrekvens
}

void spi_transmit(unsigned char data)
{
	 SPDR = data;
	 while(!(SPSR & (1<<SPIF))); //Vänta på att överföring är klar
	 trash = SPDR;
}

int main(void)
{
	spi_init();
	//PORTA = (0<<PORTA2); //Slave select
	//PORTA = (1<<PORTA0);
	while(1)
	{
	_delay_ms(500);
	PORTA = (0<<PORTA2); //Slave select
	spi_transmit(inbuffer);
	PORTA = (1<<PORTA2); //Slave deselect
	}
}