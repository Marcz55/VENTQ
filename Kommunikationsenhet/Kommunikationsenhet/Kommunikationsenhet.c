/*
 * Kommunikationsenhet.c
 *
 * Created: 3/25/2015 11:26:35 AM
 *  Author: marwa828
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define F_CPU 14745600UL

unsigned char BTinbuffer;
unsigned char BTutbuffer;
unsigned char SPIinbuffer;
unsigned char SPIutbuffer;

void Bluetooth_init()
{
	UBRRL = (0<<UBRR3)|(1<<UBRR2)|(1<<UBRR1)|(1<<UBRR0); //S�tt baud-rate till 115200 
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); //S�tt p� s�ndare och mottagare, samt s�tt p� interrupts vid recieve complete respektive tom buffer.
	UCSRC = (1<<URSEL)|(3<<UCSZ0)|(0<<UPM1)|(0<<UPM0); //S�tt 8-bit meddelanden samt ingen paritet
	DDRA = (0<<DDA1)|(1<<DDA0); //Definiera en input och en output
	PORTA = (0<<PORTA0)|(1<<PORTA1); //Skicka ut clear to send
}

void SPI_init(void)
{
	DDRB = (1<<PORTB6); //Alla utom MISO ska vara ing�ngar.
	SPCR = (1<<SPE)|(1<<SPIE); //S�tt p� SPI
	//Ska s�tta SPIE h�r med f�r att fixa avbrottstyrning
	
}

void Send(unsigned char data)

{
	while ( !( UCSRA & (1<<UDRE)));
	UDR = data;
}

unsigned char Recieve(void)
{
	return UDR;
}

unsigned char spirecieve(void)
{
	return SPDR;
}

void spiwrite(unsigned char data)
{
	SPDR = data;
}

int main(void)
{
	Bluetooth_init();
	SPI_init();
	sei();
	//unsigned char test;
	
	while(1)
	{
	MCUCR = (1<<SE); //Sleep enable
	sleep_mode();
	}

}

ISR(USART_RXC_vect) //Inkommet bluetoothmeddelande
{
	MCUCR = (0<<SE);
	SPDR = Recieve(); //Information som ska skickas �verf�rs direkt till SPDR, d�r det �r redo att f�ras �ver till masterenheten.
}

ISR(SPISTC_vect)//SPI-�verf�ring klar
{
	MCUCR = (0<<SE);
	Send(SPDR);
	SPDR = 0x44; //�terst�ll SPDR.
	
}
//Kommer att beh�va en lista d�r indata kan sparas tillf�lligt.


//Gammal version nedan:
/*
 * Kommunikationsenhet.c
 *
 * Created: 3/25/2015 11:26:35 AM
 *  Author: marwa828
 */ 


/*#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define F_CPU 14745600UL

unsigned char BTinbuffer;
unsigned char BTutbuffer;
unsigned char SPIinbuffer;
unsigned char SPIutbuffer;

void Bluetooth_init()
{
	UBRRL = (0<<UBRR3)|(1<<UBRR2)|(1<<UBRR1)|(1<<UBRR0); //S�tt baud-rate till 115200 
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); //S�tt p� s�ndare och mottagare, samt s�tt p� interrupts vid recieve complete respektive tom buffer.
	UCSRC = (1<<URSEL)|(3<<UCSZ0)|(0<<UPM1)|(0<<UPM0); //S�tt 8-bit meddelanden samt ingen paritet
	DDRA = (0<<DDA1)|(1<<DDA0); //Definiera en input och en output
	PORTA = (0<<PORTA0)|(1<<PORTA1); //Skicka ut clear to send
}

void SPI_init(void)
{
	DDRB = (1<<PORTB6); //Alla utom MISO ska vara ing�ngar.
	SPCR = (1<<SPE)|(1<<SPIE); //S�tt p� SPI
	//Ska s�tta SPIE h�r med f�r att fixa avbrottstyrning
	
}

void Send(unsigned char data)

{
	while ( !( UCSRA & (1<<UDRE)));
	UDR = data;
}

unsigned char Recieve(void)
{
	while ( !(UCSRA & (1<<RXC)) );
	return UDR;
}

unsigned char spirecieve(void)
{
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void spiwrite(unsigned char data)
{
	SPDR = data;
}

int main(void)
{
	Bluetooth_init();
	SPI_init();
	sei();
	unsigned char test;
	
	while(1)
	{
	MCUCR = (1<<SE); //Sleep enable
	sleep_mode();
	}

}

ISR(USART_RXC_vect)
{
	MCUCR = (0<<SE);
	BTinbuffer = Recieve(); //Vi kommer f�rlora information om meddelanden skickas i snabb f�ljd
}

ISR(SPISTC_vect)
{
	MCUCR = (0<<SE);
	spiwrite(BTinbuffer); //Skicka �ver inkommet meddelande till SPDR.
	BTinbuffer = 0;
	SPIinbuffer = SPDR;
	if(SPIinbuffer != 0)
	{
	Send(SPIinbuffer);
	}
}
//Kommer att beh�va en lista d�r indata kan sparas tillf�lligt.
*/