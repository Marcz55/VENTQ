
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_CPU 16000000UL
#include <util/delay.h>

unsigned char inbuffer = 0x44;

void spiInit(void)
{
	DDRA = (1<<PORTA2);
	PORTA = (1<<PORTA2); //Ha ingen slav vald
	DDRB = (1<<PORTB5)|(1<<PORTB7)|(1<<PORTB4); //Definiera outputs
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0); //Sätt enhet till master, enable spi, klockfrekvens
}

void spiTransmit(unsigned char data)
{
	 SPDR = data;
	 while(!(SPSR & (1<<SPIF))); //Vänta på att överföring är klar
	 inbuffer = SPDR;
}

int main(void)
{
	spiInit();
	PORTA = (0<<PORTA2); //Slave select
	int p = 0;
	while(p<100)
	{
	_delay_us(5);
	PORTA = (0<<PORTA2); //Slave select
	spiTransmit(0x44);
	PORTA = (1<<PORTA2); //Slave deselect
	p = p+1;
	}
	while(1)
	{
		_delay_ms(1000);
		PORTA = (0<<PORTA2); //Slave select
		spiTransmit(48);
		PORTA = (1<<PORTA2); //Slave deselect
	}
}
